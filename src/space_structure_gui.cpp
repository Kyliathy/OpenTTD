/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure_gui.cpp Native module selection, paid activation and precise Core/Dock service controls.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups2/8.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-4C/6B/7.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CD; shared heavy selection and immutable original reserved bills.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K; original recipe/input/output/work and blocked disclosures.
 */
#include "stdafx.h"
#include "space_mining_tuning.h"
#include "space_structure.h"
#include "space_construction_gui.h"
#include "space_approach.h"
#include "space_structure_cmd.h"
#include "space_vehicle.h"
#include "space_vehicle_cmd.h"
#include "space_manifest.h"
#include "orbital_manufacturing.h"
#include "orbital_recipe.h"
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
#include "widgets/space_structure_widget.h"
#include "zoom_func.h"
#include "safeguards.h"

static constexpr std::initializer_list<NWidgetPart> widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkBlue),
		NWidget(WWT_CAPTION, Colours::DarkBlue), SetStringTip(STR_SPACE_CONSTRUCTION_CAPTION),
	EndContainer(),
	NWidget(WWT_DROPDOWN, Colours::DarkBlue, WID_SC_SITE), SetStringTip(STR_SPACE_CONSTRUCTION_SELECT), SetFill(1, 0), SetResize(1, 0),
	NWidget(WWT_DROPDOWN, Colours::DarkBlue, WID_SC_RECIPE), SetStringTip(STR_SPACE_CONSTRUCTION_SELECT), SetFill(1, 0), SetResize(1, 0),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_NEXT_BUILDER), SetStringTip(STR_SPACE_CONSTRUCTION_NEXT_PROBE, STR_SPACE_CONSTRUCTION_NEXT_PROBE), SetFill(1, 0), SetResize(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_LOCATE), SetStringTip(STR_SPACE_CONSTRUCTION_LOCATE, STR_SPACE_CONSTRUCTION_LOCATE), SetFill(1, 0), SetResize(1, 0),
	EndContainer(),
	NWidget(WWT_DROPDOWN, Colours::DarkBlue, WID_SC_PAGE), SetStringTip(STR_SPACE_ACTION_CONSTRUCTION), SetFill(1, 0), SetResize(1, 0),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, Colours::DarkBlue, WID_SC_DETAILS), SetMinimalSize(320, 100), SetResize(1, 1), SetScrollbar(WID_SC_SCROLLBAR), EndContainer(),
		NWidget(NWID_VSCROLLBAR, Colours::DarkBlue, WID_SC_SCROLLBAR),
	EndContainer(),
	// Each page fills the shared action area's spare rows with the same panel
	// background; a shorter page must not expose stale pixels from the viewport.
	NWidget(NWID_SELECTION, Colours::DarkBlue, WID_SC_ACTIONS),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_PLACE), SetStringTip(STR_SPACE_CONSTRUCTION_PLACE, STR_SPACE_CONSTRUCTION_PLACE), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_QUEUE), SetStringTip(STR_SPACE_CONSTRUCTION_QUEUE, STR_SPACE_CONSTRUCTION_QUEUE), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_CANCEL), SetStringTip(STR_SPACE_CONSTRUCTION_CANCEL, STR_SPACE_CONSTRUCTION_CANCEL), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_CLEAR), SetStringTip(STR_SPACE_CONSTRUCTION_CLEAR, STR_SPACE_CONSTRUCTION_CLEAR), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_ACTIVATE), SetStringTip(STR_SPACE_CONSTRUCTION_ACTIVATE, STR_SPACE_CONSTRUCTION_ACTIVATE), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_RESERVE), SetStringTip(STR_SPACE_CONSTRUCTION_RESERVE, STR_SPACE_CONSTRUCTION_RESERVE), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_VISIT), SetStringTip(STR_SPACE_ORDER_VISIT, STR_SPACE_ORDER_VISIT), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(WWT_PANEL, Colours::DarkBlue), SetFill(1, 1), SetResize(1, 0), EndContainer(),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_LOAD_STEEL), SetStringTip(STR_SPACE_CONSTRUCTION_LOAD_STEEL, STR_SPACE_CONSTRUCTION_LOAD_STEEL), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_LOAD_GOODS), SetStringTip(STR_SPACE_CONSTRUCTION_LOAD_GOODS, STR_SPACE_CONSTRUCTION_LOAD_GOODS), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_UNLOAD), SetStringTip(STR_SPACE_CONSTRUCTION_UNLOAD, STR_SPACE_CONSTRUCTION_UNLOAD), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_COLLECT_STEEL), SetStringTip(STR_SPACE_CONSTRUCTION_COLLECT_STEEL, STR_SPACE_CONSTRUCTION_COLLECT_STEEL), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_COLLECT_GOODS), SetStringTip(STR_SPACE_CONSTRUCTION_COLLECT_GOODS, STR_SPACE_CONSTRUCTION_COLLECT_GOODS), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_UNLOAD_METAL), SetStringTip(STR_CORE_SERVICE_UNLOAD_METAL, STR_CORE_SERVICE_UNLOAD_METAL), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_UNLOAD_ELECTRONICS), SetStringTip(STR_CORE_SERVICE_UNLOAD_ELECTRONICS, STR_CORE_SERVICE_UNLOAD_ELECTRONICS), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_LOAD_METAL), SetStringTip(STR_CORE_SERVICE_LOAD_METAL, STR_CORE_SERVICE_LOAD_METAL), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_LOAD_ELECTRONICS), SetStringTip(STR_CORE_SERVICE_LOAD_ELECTRONICS, STR_CORE_SERVICE_LOAD_ELECTRONICS), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(WWT_PANEL, Colours::DarkBlue), SetFill(1, 1), SetResize(1, 0), EndContainer(),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_NEXT_SERVICE), SetStringTip(STR_CORE_SERVICE_NEXT, STR_CORE_SERVICE_NEXT), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_NEXT_BERTH), SetStringTip(STR_SPACE_DOCK_NEXT_BERTH, STR_SPACE_DOCK_NEXT_BERTH), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_START_SERVICE), SetStringTip(STR_CORE_SERVICE_START, STR_CORE_SERVICE_START), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_CANCEL_SERVICE), SetStringTip(STR_CORE_SERVICE_CANCEL, STR_CORE_SERVICE_CANCEL), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_PAUSE_SERVICE), SetStringTip(STR_CORE_SERVICE_PAUSE, STR_CORE_SERVICE_PAUSE), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_RESUME_SERVICE), SetStringTip(STR_CORE_SERVICE_RESUME, STR_CORE_SERVICE_RESUME), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_TOGGLE_PANEL), SetStringTip(STR_CORE_PANEL_TOGGLE, STR_CORE_PANEL_TOGGLE), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_COMMISSION), SetStringTip(STR_CORE_SERVICE_COMMISSION, STR_CORE_SERVICE_COMMISSION), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(WWT_PANEL, Colours::DarkBlue), SetFill(1, 1), SetResize(1, 0), EndContainer(),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_CONNECT), SetStringTip(STR_SPACE_DOCK_CONNECT, STR_SPACE_DOCK_CONNECT), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_NEXT_SOURCE), SetStringTip(STR_SPACE_DOCK_NEXT_SOURCE, STR_SPACE_DOCK_NEXT_SOURCE), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_DISCONNECT), SetStringTip(STR_SPACE_DOCK_DISCONNECT, STR_SPACE_DOCK_DISCONNECT), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_REMOVE), SetStringTip(STR_SPACE_MODULE_REMOVE, STR_SPACE_MODULE_REMOVE), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_EVACUATE), SetStringTip(STR_SPACE_MODULE_EVACUATE, STR_SPACE_MODULE_EVACUATE), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_NEXT_DESTINATION), SetStringTip(STR_SPACE_MODULE_EVACUATE_NEXT, STR_SPACE_MODULE_EVACUATE_NEXT), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(WWT_PANEL, Colours::DarkBlue), SetFill(1, 1), SetResize(1, 0), EndContainer(),
		EndContainer(),
		NWidget(NWID_VERTICAL),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_NEXT_MODEL), SetStringTip(STR_SPACE_MANUFACTURE_NEXT_MODEL, STR_SPACE_MANUFACTURE_NEXT_MODEL), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_NEXT_INPUT_SOURCE), SetStringTip(STR_SPACE_MANUFACTURE_NEXT_SOURCE, STR_SPACE_MANUFACTURE_NEXT_SOURCE), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_QUEUE_MANUFACTURE), SetStringTip(STR_SPACE_MANUFACTURE_QUEUE, STR_SPACE_MANUFACTURE_QUEUE), SetFill(1, 0), SetResize(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::DarkBlue, WID_SC_CANCEL_MANUFACTURE), SetStringTip(STR_SPACE_MANUFACTURE_CANCEL, STR_SPACE_MANUFACTURE_CANCEL), SetFill(1, 0), SetResize(1, 0),
			EndContainer(),
			NWidget(WWT_PANEL, Colours::DarkBlue), SetFill(1, 1), SetResize(1, 0), EndContainer(),
		EndContainer(),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_PANEL, Colours::DarkBlue), SetFill(1, 0), SetResize(1, 0), EndContainer(),
		NWidget(WWT_RESIZEBOX, Colours::DarkBlue, WID_SC_RESIZE),
	EndContainer(),
};
static WindowDesc desc(WindowPosition::Automatic, "space_construction", 560, 380, WindowClass::SpaceConstruction, WindowClass::None, {}, widgets);
/** @param page Presentation-only action category. @return Its localized visible selector label. */
static StringID PageName(SpaceConstructionPage page)
{
	static constexpr StringID names[] = {STR_SPACE_ACTION_CONSTRUCTION, STR_SPACE_ACTION_INVENTORY, STR_SPACE_ACTION_SERVICE,
		STR_SPACE_ACTION_CONNECTIONS, STR_SPACE_ACTION_MANUFACTURE};
	return names[to_underlying(page)];
}
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
/** @param model Immutable supported recipe identity. @return Translated player-facing vehicle role without requiring a materialized Engine. */
static StringID ManufactureModelName(uint16_t model)
{
	switch (model) {
		case 15: return STR_SPACE_MANUFACTURE_MINING;
		case 19: return STR_SPACE_MANUFACTURE_SURVEY;
		case 20: return STR_SPACE_MANUFACTURE_BUILDER;
		case 16: return STR_SPACE_MANUFACTURE_HEAVY;
		case 17: return STR_SPACE_MANUFACTURE_HAULER;
		case SPACE_HEAVY_MINER_MODEL: return STR_SPACE_MANUFACTURE_HEAVY_MINING;
		default: return STR_SPACE_MANUFACTURE_NEXT_MODEL;
	}
}
/** @param block Exact pure native removal admission. @return Player-facing action needed before this module can be removed. */
static StringID OrbitalRemovalReason(OrbitalRemovalBlock block)
{
	static constexpr StringID reasons[] = {STR_SPACE_MODULE_REMOVE_READY, STR_SPACE_MODULE_REMOVE_OWNER, STR_SPACE_MODULE_REMOVE_STOCK,
		STR_SPACE_MODULE_REMOVE_SERVICE, STR_SPACE_MODULE_REMOVE_CONSTRUCTION, STR_SPACE_MODULE_REMOVE_VEHICLE, STR_SPACE_MODULE_REMOVE_DEPENDENT,
		STR_SPACE_MODULE_REMOVE_CONNECTION, STR_SPACE_MODULE_REMOVE_UTILITY, STR_SPACE_MODULE_REMOVE_HISTORY};
	static_assert(std::size(reasons) == to_underlying(OrbitalRemovalBlock::End));
	assert(block < OrbitalRemovalBlock::End);
	return reasons[to_underlying(block)];
}
/** @param reason Original owner's prerequisite. @return Parameter-free native StringID suitable for a {STRING} argument. */
static StringID ConstructionReasonString(OrbitalConstructionBlock reason)
{
	static constexpr StringID reasons[] = {STR_SPACE_BUILD_NO_ORDER, STR_SPACE_BUILD_TARGET_LOST, STR_SPACE_BUILD_COMPLETE,
		STR_SPACE_BUILD_TIMEOUT, STR_SPACE_BUILD_UNAVAILABLE, STR_SPACE_BUILD_STOPPED, STR_SPACE_BUILD_SERVICE,
		STR_SPACE_BUILD_PAUSED, STR_SPACE_BUILD_APPROACH, STR_SPACE_BUILD_TRAVEL, STR_SPACE_BUILD_MATERIALS,
		STR_SPACE_BUILD_CORE, STR_SPACE_BUILD_WORKER, STR_SPACE_BUILD_POWER, STR_SPACE_BUILD_READY, STR_SPACE_BUILD_WORKING};
	static_assert(std::size(reasons) == to_underlying(OrbitalConstructionBlock::End));
	assert(reason < OrbitalConstructionBlock::End);
	return reasons[to_underlying(reason)];
}

/** @copydoc GetSpaceConstructionReasonText */
std::string GetSpaceConstructionReasonText(OrbitalConstructionBlock reason) { return GetString(ConstructionReasonString(reason)); }

/** @copydoc GetSpaceConstructionBuilderText */
std::vector<std::string> GetSpaceConstructionBuilderText(const SpaceVehicle &builder)
{
	const auto readiness = GetOrbitalConstructionReadiness(builder);
	std::vector<std::string> lines{GetString(STR_SPACE_BUILD_STATUS, builder.index, ConstructionReasonString(readiness.block))};
	const auto &queue = builder.space_orders;
	if (queue.cursor < queue.entries.size()) {
		const auto &order = queue.entries[queue.cursor];
		static constexpr StringID actions[] = {STR_SPACE_ORDER_VISIT, STR_SPACE_ORDER_SURVEY, STR_SPACE_ORDER_MINE, STR_SPACE_ORDER_CONSTRUCT, STR_SPACE_ORDER_UNLOAD, STR_SPACE_ORDER_SERVICE};
		static constexpr StringID targets[] = {STR_SPACE_TARGET_BODY, STR_SPACE_TARGET_STRUCTURE, STR_SPACE_TARGET_BLUEPRINT, STR_SPACE_TARGET_CENTER, STR_SPACE_TARGET_CRAFT};
		const bool site = order.target.kind == SpaceOrderTargetKind::Structure || order.target.kind == SpaceOrderTargetKind::Blueprint;
		lines.push_back(GetString(STR_SPACE_BUILD_CURRENT_ORDER, queue.cursor + 1, actions[to_underlying(order.action)], targets[to_underlying(order.target.kind)], uint64_t{order.target.id} + (site ? 1 : 0)));
	} else {
		// The selected window site may differ from the builder's completed job;
		// retain that job's identity so its completion cannot be misattributed.
		if (readiness.block == OrbitalConstructionBlock::Completed) lines.push_back(GetString(STR_SPACE_ORDER_ROW,
			queue.entries.size(), STR_SPACE_ORDER_CONSTRUCT, STR_SPACE_TARGET_BLUEPRINT, readiness.target.structure.base() + 1));
		lines.push_back(GetString(queue.entries.empty() ? STR_SPACE_BUILD_QUEUE_EMPTY : STR_SPACE_BUILD_QUEUE_FINISHED));
	}
	return lines;
}

/** @copydoc GetSpaceConstructionMaterialText */
std::vector<std::string> GetSpaceConstructionMaterialText(const SpaceStructure &site, const SpaceVehicle *builder)
{
	const auto &recipe = *GetSpaceConstructionDefinition(site.state.kind);
	std::vector<std::string> lines;
	if (site.state.phase == SpaceStructurePhase::FrameComplete) lines.push_back(GetString(STR_SPACE_BUILD_COMPLETE));
	lines.push_back(GetString(STR_SPACE_CONSTRUCTION_STOCK, site.job.inputs[0].TotalCount(), recipe.inputs[0].quantity,
		site.job.inputs[1].TotalCount(), recipe.inputs[1].quantity, site.job.work_ticks, recipe.work_ticks));
	lines.push_back(GetString(STR_SPACE_CONSTRUCTION_CONSUMED, site.job.consumed[0], site.job.consumed[1]));
	lines.push_back(GetString(STR_SPACE_CONSTRUCTION_REMAINING,
		recipe.inputs[0].quantity - site.job.inputs[0].TotalCount() - site.job.consumed[0],
		recipe.inputs[1].quantity - site.job.inputs[1].TotalCount() - site.job.consumed[1]));
	// The site owner already combines delivered and internally reserved packets;
	// the builder's still-carried freight is disclosed separately, never counted
	// as another copy of either site's reservation or completion consumption.
	if (builder != nullptr && builder->cargo.TotalCount() != 0) lines.push_back(GetString(STR_SPACE_BUILD_CARRIED, builder->cargo.TotalCount(), CargoSpec::Get(builder->cargo_type)->name));
	return lines;
}

/** Fund Industry-style native selector; all mutations pass through synchronized commands. */
struct SpaceConstructionWindow : Window {
	SpaceBuildKind kind = SpaceBuildKind::Core;
	SpaceStructureRef selected;
	SpaceCraftRef builder;
	SpaceCraftRef service_craft;
	uint8_t service_berth = 0; ///< Presentation-only selected contained job; native commands still identify its exact saved craft.
	SpaceStructureRef utility_source_selection; ///< Presentation-only source proposal; only the synchronized connection command publishes it.
	SpaceStructureRef evacuation_destination; ///< Presentation-only packet destination; quotes/selection cannot move cargo or cancel a job.
	/** One wrapped display row; a site reference enables selection but grants no work. */
	struct DisplayLine {
		std::string text; ///< Localized read-only description.
		TextColour colour; ///< Selection highlighting only.
		SpaceStructureRef site; ///< Exact incarnation, or empty for a detail row.
		int top = 0, height = 0; ///< Scroll-content pixel bounds, rebuilt on resize.
	};
	std::vector<DisplayLine> lines;
	SpaceConstructionPage page = SpaceConstructionPage::Construction;
	std::vector<SpaceStructureRef> site_choices; ///< Captured dropdown incarnations, never guessed pool IDs.
	Scrollbar *scroll = nullptr;
	uint16_t manufacture_model = 15; ///< Presentation-only recipe proposal; synchronized admission owns the actual job.
	SpaceStructureRef manufacture_source; ///< Explicit adjacent inventory proposal, never an implicit packet reservation.
	/** Refresh presentation only. @param elapsed Window intervals; never simulation work or cargo age. */
	const IntervalTimer<TimerWindow> refresh = {std::chrono::milliseconds(250), [this]([[maybe_unused]] auto elapsed) { this->RefreshPresentation(); this->SetDirty(); }};
	/** Initialize native controls and select the first owned orbital builder. */
	SpaceConstructionWindow() : Window(desc) { this->InitNested(0); this->scroll = this->GetScrollbar(WID_SC_SCROLLBAR); this->NextBuilder(); this->NextServiceCraft(); this->RefreshPresentation(); }
	/**
	 * Bound text controls to the available screen; full labels remain in tooltips.
	 * @param widget Native presentation/action identity.
	 * @param size Measured minimum size, including the current font and scale.
	 * @param padding Framework padding, already included in the measured size.
	 * @param fill Framework fill increments, unchanged.
	 * @param resize Framework resize increments, unchanged.
	 */
	void UpdateWidgetSize(WidgetID widget, Dimension &size, [[maybe_unused]] const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		const uint available = std::max(100, _screen.width - ScaleGUITrad(16));
		if (widget == WID_SC_DETAILS) {
			// Preserve a usable text viewport while allowing the fixed action rows
			// to fit after a resolution/font change. Content remains scrollable even
			// when large text leaves room for only a few lines at minimum size.
			const uint minimum_text = GetCharacterHeight(FontSize::Normal) + ScaleGUITrad(8);
			const uint share = std::max(1, (GetMainViewBottom() - GetMainViewTop()) / 8);
			size.height = std::min(size.height, std::max(minimum_text, share));
		}
		// Action pairs must remain reachable with long translations and large
		// fonts. Ellipsis affects only labels; commands keep their original IDs.
		if (widget > WID_SC_DETAILS && widget <= WID_SC_CANCEL_MANUFACTURE && widget != WID_SC_SCROLLBAR && widget != WID_SC_RECIPE) size.width = std::min(size.width, available / 2);
		// Fixed selectors span the complete window instead of reserving a column;
		// their selected labels must not enlarge every action page beyond the screen.
		if (widget == WID_SC_SITE || widget == WID_SC_PAGE || widget == WID_SC_RECIPE) size.width = std::min(size.width, available);
	}
	/**
	 * Keep selected site, builder and action category visible above the scroll area.
	 * @param widget Control whose label is requested.
	 * @param stringid Its ordinary fallback string.
	 * @return Localized presentation of the retained exact selection.
	 */
	std::string GetWidgetString(WidgetID widget, StringID stringid) const override
	{
		if (widget == WID_SC_PAGE) return GetString(PageName(this->page));
		if (widget == WID_SC_SITE) {
			const auto *site = ResolveSpaceStructure(this->selected);
			return site == nullptr ? GetString(STR_SPACE_CONSTRUCTION_SELECT) : GetString(STR_SPACE_CONSTRUCTION_SELECTED, ModuleName(site->state.kind), site->index.base() + 1);
		}
		if (widget == WID_SC_NEXT_BUILDER) {
			const auto *craft = ResolveSpaceCraft(this->builder);
			return craft == nullptr ? GetString(stringid) : GetString(STR_SPACE_ACTION_BUILDER, craft->index);
		}
		return this->Window::GetWidgetString(widget, stringid);
	}
	/** Recompute wrapped pixel ranges and clamp scrolling after content/geometry changes. */
	void ReflowLines()
	{
		if (this->scroll == nullptr) return;
		const auto *panel = this->GetWidget<NWidgetBase>(WID_SC_DETAILS);
		const int padding = ScaleGUITrad(4);
		const int width = std::max(1, static_cast<int>(panel->current_x) - 2 * padding);
		int top = 0;
		// Wrapping belongs to layout, not painting. One pixel-based scrollbar
		// covers every detail and site row, including unusually long translations.
		for (auto &line : this->lines) {
			line.top = top;
			line.height = GetStringHeight(line.text, width) + ScaleGUITrad(2);
			top += line.height;
		}
		this->scroll->SetCount(top);
		this->scroll->SetCapacity(std::max(1, static_cast<int>(panel->current_y) - 2 * padding));
		this->scroll->SetStepSize(GetCharacterHeight(FontSize::Normal) + ScaleGUITrad(2));
	}
	/** Fit restored/resized geometry between the native toolbar and status bar. */
	void OnResize() override
	{
		const int available_height = GetMainViewBottom() - GetMainViewTop();
		const int target_width = std::max<int>(this->nested_root->smallest_x, std::min<int>(this->width, _screen.width));
		const int target_height = std::max<int>(this->nested_root->smallest_y, std::min<int>(this->height, available_height));
		// A previous larger resolution must not strand the action pages offscreen;
		// ResizeWindow re-enters once with the clamped dimensions, then converges.
		if (target_width != this->width || target_height != this->height) {
			ResizeWindow(this, target_width - this->width, target_height - this->height, false, false);
			return;
		}
		this->left = std::clamp(this->left, 0, std::max(0, _screen.width - this->width));
		this->top = std::clamp(this->top, GetMainViewTop(), std::max(GetMainViewTop(), GetMainViewBottom() - this->height));
		this->ReflowLines();
	}
	/** Draw retained presentation only; refresh timers and input own layout/selection updates. */
	void OnPaint() override { this->DrawWidgets(); }
	/**
	 * Draw only the visible portion of the complete wrapped content.
	 * @param rect Native panel rectangle in window coordinates.
	 * @param widget Widget being painted; other controls use the normal renderer.
	 */
	void DrawWidget(const Rect &rect, WidgetID widget) const override
	{
		if (widget != WID_SC_DETAILS) return;
		const Rect inner = rect.Shrink(ScaleGUITrad(4));
		DrawPixelInfo clipped;
		if (!FillDrawPixelInfo(&clipped, inner)) return;
		AutoRestoreBackup drawing(_cur_dpi, &clipped);
		// Physical clipping keeps overflowing details out of the action controls;
		// the saved scroll offset selects content without changing any owner.
		for (const auto &line : this->lines) {
			const int y = line.top - this->scroll->GetPosition();
			if (y + line.height <= 0) continue;
			if (y >= inner.Height()) break;
			DrawStringMultiLine(0, inner.Width() - 1, y, y + line.height - 1, line.text, line.colour);
		}
	}
	/** Cycle actual orbital probes independently of construction-builder selection. */
	void NextServiceCraft()
	{
		std::vector<SpaceCraftRef> craft;
		// The service selector admits only actual owned probe hulls; rockets/carried payloads cannot occupy this berth.
		for (const auto *v : SpaceVehicle::Iterate()) {
			const auto model = GetSpaceVehicleModel(*v);
			if (v->owner == _local_company && v->construction.placement == SpacePlacement::Orbital && GetSpaceServiceTuning(model)) craft.push_back(GetSpaceCraftReference(*v));
		}
		const auto it = std::find(craft.begin(), craft.end(), this->service_craft);
		this->service_craft = craft.empty() ? SpaceCraftRef{} : (it == craft.end() || std::next(it) == craft.end() ? craft.front() : *std::next(it));
		const auto *site = ResolveSpaceStructure(this->selected); const auto *probe = ResolveSpaceCraft(this->service_craft);
		// Selecting an actual admitted probe follows its physical Dock berth without changing the job or its reservation.
		if (site != nullptr && probe != nullptr && site->state.kind == SpaceBuildKind::Dock) {
			if (const auto berth = GetSpaceStructureBerth(*probe, *site)) this->service_berth = *berth;
		}
	}
	/** Cycle actual adjacent original owned panel proposals; this selector registers no connection or energy. */
	void NextUtilitySource()
	{
		const auto *site = ResolveSpaceStructure(this->selected);
		if (site == nullptr) { this->utility_source_selection = {}; return; }
		std::vector<SpaceStructureRef> sources;
		AutoRestoreBackup actor(_current_company, _local_company);
		// Native pure admission supplies the complete ownership/activation/physical-edge test for every candidate panel.
		for (const auto *source : SpaceStructure::Iterate()) {
			if (CmdConnectOrbitalServices({}, site->index, site->generation, source->index, source->generation).Succeeded()) sources.push_back(GetSpaceStructureReference(*source));
		}
		const auto it = std::find(sources.begin(), sources.end(), this->utility_source_selection);
		this->utility_source_selection = sources.empty() ? SpaceStructureRef{} : (it == sources.end() || std::next(it) == sources.end() ? sources.front() : *std::next(it));
	}
	/** Cycle only actual adjacent destinations that admit the complete physical packet/job evacuation. */
	void NextEvacuationDestination()
	{
		const auto *site = ResolveSpaceStructure(this->selected);
		if (site == nullptr) { this->evacuation_destination = {}; return; }
		std::vector<SpaceStructureRef> destinations;
		AutoRestoreBackup actor(_current_company, _local_company);
		// Whole pure command admission excludes full/unowned/inert destinations and stock that would lose its last Core after removal.
		for (const auto *destination : SpaceStructure::Iterate()) {
			if (CmdEvacuateOrbitalModule({}, site->index, site->generation, destination->index, destination->generation).Succeeded()) destinations.push_back(GetSpaceStructureReference(*destination));
		}
		const auto it = std::find(destinations.begin(), destinations.end(), this->evacuation_destination);
		this->evacuation_destination = destinations.empty() ? SpaceStructureRef{} : (it == destinations.end() || std::next(it) == destinations.end() ? destinations.front() : *std::next(it));
	}
	/** Cycle actual adjacent owned input/cancellation inventories without reserving their packets. */
	void NextManufactureSource()
	{
		const auto *site = ResolveSpaceStructure(this->selected);
		if (site == nullptr) { this->manufacture_source = {}; return; }
		std::vector<SpaceStructureRef> sources;
		// Proposals may name an empty genuine inventory so the player can plan deliveries; only the command's full recipe/funds/power/berth check enables a request.
		for (const auto *source : SpaceStructure::Iterate()) {
			if (source->state.owner != _local_company || (source->state.kind != SpaceBuildKind::Core && source->state.kind != SpaceBuildKind::Storage) ||
				!IsOrbitalModuleOperational(*source) || !AreOrbitalModuleFootprintsAdjacent(site->state, source->state)) continue;
			sources.push_back(GetSpaceStructureReference(*source));
		}
		const auto current = std::ranges::find(sources, this->manufacture_source);
		this->manufacture_source = sources.empty() ? SpaceStructureRef{} : (current == sources.end() || std::next(current) == sources.end() ? sources.front() : *std::next(current));
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
	void RefreshPresentation()
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
		this->SetWidgetDisabledState(WID_SC_QUEUE, !owned || v == nullptr || v->owner != _local_company || site->state.phase != SpaceStructurePhase::Blueprint);
		this->SetWidgetDisabledState(WID_SC_CANCEL, !owned || site->state.phase != SpaceStructurePhase::Blueprint);
		this->SetWidgetDisabledState(WID_SC_CLEAR, v == nullptr || v->owner != _local_company);
		this->SetWidgetDisabledState(WID_SC_VISIT, !owned || v == nullptr || v->owner != _local_company);
		this->SetWidgetDisabledState(WID_SC_LOCATE, site == nullptr);
		// Collection is restricted to actual unconsumed cancelled stock; activation never grants a refund path.
		for (uint i = 0; i < 2; ++i) this->SetWidgetDisabledState(WID_SC_COLLECT_STEEL + i, !owned || v == nullptr || site->state.phase != SpaceStructurePhase::Cancelled || site->job.inputs[i].TotalCount() == 0);
		bool can_activate = false;
		if (owned) {
			AutoRestoreBackup actor(_current_company, _local_company);
			can_activate = CmdActivateOrbitalModule({}, site->index, site->generation).Succeeded();
		}
		this->SetWidgetDisabledState(WID_SC_ACTIVATE, !can_activate);
		const bool exchange = owned && v != nullptr && IsOrbitalModuleOperational(*site) &&
			(site->state.kind == SpaceBuildKind::Dock ? GetSpaceStructureBerth(*v, *site).has_value() : v->position == GetSpaceStructureApproach(site->state));
		// Module exchange is a precise approach action, independent of the launch center's twenty-tile drop circle.
		for (uint widget = WID_SC_LOAD_STEEL; widget <= WID_SC_UNLOAD; ++widget) this->SetWidgetDisabledState(widget, !exchange);
		const auto *probe = ResolveSpaceCraft(this->service_craft);
		// Removed/foreign service selections are refreshed without changing the contained job's retained craft reference.
		if (probe == nullptr || probe->owner != _local_company) {
			const auto selected_berth = this->service_berth;
			// Refreshing a lost hull must retain the chosen accountable job, so its held parts remain reachable through Cancel.
			const bool held = site != nullptr && site->module != nullptr && selected_berth < site->module->GetServiceJobs().size() &&
				site->module->GetServiceJobs()[selected_berth]->remaining_ticks != 0;
			this->service_craft = {}; this->NextServiceCraft(); probe = ResolveSpaceCraft(this->service_craft);
			if (held) this->service_berth = selected_berth;
		}
		const bool operational = owned && IsOrbitalModuleOperational(*site);
		// Changing from a Dock to a single-berth module cannot retain its second slot.
		if (site != nullptr && this->service_berth >= GetOrbitalModuleServiceCapacity(site->state.kind)) this->service_berth = 0;
		// Paid retained jobs remain cancellable during a dependency/power outage; selecting a berth grants no service work.
		const bool retained = owned && site->module != nullptr && site->module->activation_ticks == 0;
		const bool busy = retained && site->module->GetServiceJobs()[this->service_berth]->remaining_ticks != 0;
		bool can_start = false;
		// The Start button uses the same pure whole admission as execution, including both real parts and the precise paid hull.
		if (operational && probe != nullptr) {
			AutoRestoreBackup actor(_current_company, _local_company);
			const bool selected_berth = site->state.kind != SpaceBuildKind::Dock || GetSpaceStructureBerth(*probe, *site) == std::optional<uint8_t>{this->service_berth};
			can_start = selected_berth && CmdServiceOrbitalProbe({}, site->index, site->generation, probe->index, probe->generation, OrbitalServiceControl::Start).Succeeded();
		}
		this->SetWidgetDisabledState(WID_SC_START_SERVICE, !can_start);
		// Pause/resume/cancel act on the actual occupant even when a different probe is selected or the original hull was lost.
		for (uint widget = WID_SC_PAUSE_SERVICE; widget <= WID_SC_CANCEL_SERVICE; ++widget) this->SetWidgetDisabledState(widget, !busy);
		this->SetWidgetDisabledState(WID_SC_TOGGLE_PANEL, !operational || site->module->producer == UtilityProducerID{UINT32_MAX});
		bool can_commission = false;
		if (operational) {
			AutoRestoreBackup actor(_current_company, _local_company);
			can_commission = CmdCommissionOrbitalServices({}, site->index, site->generation).Succeeded();
		}
		this->SetWidgetDisabledState(WID_SC_COMMISSION, !can_commission);
		// Every part-exchange button also preflights its actual source and destination; empty/full/stopped/remote owners stay visibly disabled.
		for (uint widget = WID_SC_UNLOAD_METAL; widget <= WID_SC_LOAD_ELECTRONICS; ++widget) {
			bool allowed = false;
			if (operational && probe != nullptr) {
				AutoRestoreBackup actor(_current_company, _local_company);
				allowed = CmdMoveOrbitalSupply({}, site->index, site->generation, probe->index, probe->generation,
					static_cast<uint8_t>(1 + (widget - WID_SC_UNLOAD_METAL) % 2), 1, widget >= WID_SC_LOAD_METAL).Succeeded();
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
		this->SetWidgetDisabledState(WID_SC_RESERVE, !can_reserve);
		bool can_connect = false;
		// Source selection remains a proposal until the native command atomically publishes the original connection/consumer.
		if (operational && (site->state.kind == SpaceBuildKind::Dock || site->state.kind == SpaceBuildKind::Assembly) && site->module->utility_source == SpaceStructureRef{}) {
			AutoRestoreBackup actor(_current_company, _local_company);
			can_connect = CmdConnectOrbitalServices({}, site->index, site->generation, this->utility_source_selection.structure, this->utility_source_selection.generation).Succeeded();
			if (!can_connect) {
				this->NextUtilitySource();
				can_connect = CmdConnectOrbitalServices({}, site->index, site->generation, this->utility_source_selection.structure, this->utility_source_selection.generation).Succeeded();
			}
		}
		this->SetWidgetDisabledState(WID_SC_CONNECT, !can_connect);
		this->SetWidgetDisabledState(WID_SC_NEXT_BERTH, !retained || site->state.kind != SpaceBuildKind::Dock);
		this->SetWidgetDisabledState(WID_SC_NEXT_SOURCE, !can_connect);
		bool can_disconnect = false;
		if (retained) {
			AutoRestoreBackup actor(_current_company, _local_company);
			can_disconnect = CmdDisconnectOrbitalServices({}, site->index, site->generation).Succeeded();
		}
		this->SetWidgetDisabledState(WID_SC_DISCONNECT, !can_disconnect);
		// Demolition uses the same whole pure admission as execution; stock, held jobs and actual occupancy cannot disappear through a UI shortcut.
		this->SetWidgetDisabledState(WID_SC_REMOVE, !owned || ClassifyOrbitalModuleRemoval(*site, _local_company) != OrbitalRemovalBlock::None);
		bool can_evacuate = false;
		if (owned) {
			AutoRestoreBackup actor(_current_company, _local_company);
			can_evacuate = CmdEvacuateOrbitalModule({}, site->index, site->generation, this->evacuation_destination.structure, this->evacuation_destination.generation).Succeeded();
			if (!can_evacuate) {
				this->NextEvacuationDestination();
				can_evacuate = CmdEvacuateOrbitalModule({}, site->index, site->generation, this->evacuation_destination.structure, this->evacuation_destination.generation).Succeeded();
			}
		}
		this->SetWidgetDisabledState(WID_SC_EVACUATE, !can_evacuate); this->SetWidgetDisabledState(WID_SC_NEXT_DESTINATION, !can_evacuate);
		const bool manufacture_owned = retained && site->state.kind == SpaceBuildKind::Assembly;
		bool can_manufacture = false, can_cancel_manufacture = false;
		if (manufacture_owned) {
			const auto *source = ResolveSpaceStructure(this->manufacture_source);
			// A changed selection or source incarnation cannot retain another site's inventory as a usable manufacturing proposal.
			if (source == nullptr || source->state.owner != _local_company || !AreOrbitalModuleFootprintsAdjacent(site->state, source->state)) this->NextManufactureSource();
			AutoRestoreBackup actor(_current_company, _local_company);
			can_manufacture = CmdQueueOrbitalManufacture({}, site->index, site->generation, this->manufacture_source.structure, this->manufacture_source.generation, this->manufacture_model).Succeeded();
			if (site->module->manufacture) {
				this->manufacture_model = site->module->manufacture->model;
				can_cancel_manufacture = CmdCancelOrbitalManufacture({}, site->index, site->generation, this->manufacture_source.structure, this->manufacture_source.generation).Succeeded();
			}
		}
		this->SetWidgetDisabledState(WID_SC_NEXT_MODEL, !manufacture_owned || site->module->manufacture != nullptr);
		this->SetWidgetDisabledState(WID_SC_NEXT_INPUT_SOURCE, !manufacture_owned);
		this->SetWidgetDisabledState(WID_SC_QUEUE_MANUFACTURE, !can_manufacture);
		this->SetWidgetDisabledState(WID_SC_CANCEL_MANUFACTURE, !can_cancel_manufacture);
		this->RebuildLines();
		this->ReflowLines();
	}
	/** Collect localized owner queries without changing simulation or drawing. */
	void RebuildLines()
	{
		this->lines.clear();
		/** @param text Translated presentation. @param colour Row colour; highlights never change gameplay ownership. */
		auto draw = [&](const std::string &text, TextColour colour = TextColour::Black) { this->lines.push_back({text, colour, {}}); };
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
		// Show the selected builder's actual current action, even when it targets a
		// different site. Selecting a blueprint cannot imply an existing assignment.
		if (v != nullptr) for (const auto &line : GetSpaceConstructionBuilderText(*v)) draw(line);
		// Only an unfinished selected site can receive another Construct action;
		// completed/cancelled frames retain their material history without a queue hint.
		if (site != nullptr && v != nullptr && site->state.phase == SpaceStructurePhase::Blueprint) draw(GetString(STR_SPACE_BUILD_QUEUE_TARGET, v->index, ModuleName(site->state.kind), site->index.base() + 1));
		if (site != nullptr) {
			draw(GetString(STR_SPACE_CONSTRUCTION_SELECTED, ModuleName(site->state.kind), site->index.base() + 1));
			// Shared wording reads the same sole job/packet owners in both views.
			for (const auto &line : GetSpaceConstructionMaterialText(*site, v)) draw(line);
			const auto removal = ClassifyOrbitalModuleRemoval(*site, _local_company);
			draw(GetString(OrbitalRemovalReason(removal)));
			if (this->evacuation_destination != SpaceStructureRef{}) draw(GetString(STR_SPACE_MODULE_EVACUATE_DESTINATION, this->evacuation_destination.structure.base() + 1));
			// Missing cargo capacity matters only for actual held inventory/jobs; an empty occupied berth instead needs real craft departure.
			else if (removal == OrbitalRemovalBlock::Inventory || removal == OrbitalRemovalBlock::Service || removal == OrbitalRemovalBlock::Construction) draw(GetString(STR_SPACE_MODULE_EVACUATE_MISSING));
			if (site->module) {
				if (site->module->activation_ticks != 0) draw(GetString(STR_SPACE_CONSTRUCTION_ACTIVATING, site->module->activation_ticks));
				else if (!IsOrbitalModuleOperational(*site)) draw(GetString(STR_SPACE_CONSTRUCTION_CORE_EDGE));
				else if (site->state.kind == SpaceBuildKind::Power) draw(GetString(STR_SPACE_CONSTRUCTION_PANEL_OUTPUT, GetOrbitalModulePanelRate(*site) / 1000));
				else if (site->state.kind == SpaceBuildKind::Assembly) draw(GetString(STR_SPACE_ASSEMBLY_READY));
				else draw(GetString(STR_SPACE_CONSTRUCTION_OPERATIONAL, GetOrbitalModuleCargoCapacity(*site, GetCargoTypeByLabel(CargoLabel{"STEL"}))));
				// The Assembly has recipe-owned inputs rather than general freight storage; showing a zero-capacity stock exchange would mislead its operator.
				if (site->state.kind != SpaceBuildKind::Assembly) draw(GetString(STR_SPACE_CONSTRUCTION_INVENTORY, site->module->stock[GetCargoTypeByLabel(CargoLabel{"STEL"})].TotalCount(), site->module->stock[GetCargoTypeByLabel(CargoLabel{"GOOD"})].TotalCount()));
				const auto &module = *site->module;
				if (site->state.kind == SpaceBuildKind::Assembly) {
					// Manufacturing source selection uses the same native connection command as Dock work, without granting service or general cargo storage.
					if (module.utility_source != SpaceStructureRef{}) draw(GetString(STR_SPACE_DOCK_SOURCE, module.utility_source.structure.base() + 1));
					else if (this->utility_source_selection != SpaceStructureRef{}) draw(GetString(STR_SPACE_DOCK_PROPOSE_SOURCE, this->utility_source_selection.structure.base() + 1));
					else draw(GetString(STR_SPACE_DOCK_SOURCE_MISSING));
					const auto *source = GetOrbitalServicePowerSite(*site);
					draw(GetString(STR_SPACE_CONSTRUCTION_PANEL_OUTPUT, source == nullptr ? 0 : GetOrbitalModulePanelRate(*source) / 1000));
					// A retained request owns its immutable recipe/source. Changing a UI
					// proposal must not relabel its actual reserved bill or work target.
					const auto model = module.manufacture ? module.manufacture->model : this->manufacture_model;
					const auto &recipe = *GetSpaceVehicleConstruction(model);
					draw(GetString(STR_SPACE_MANUFACTURE_RECIPE, ManufactureModelName(model), recipe.capital));
					const auto *inventory = ResolveSpaceStructure(module.manufacture ? module.manufacture->source : this->manufacture_source);
					if (inventory != nullptr) draw(GetString(STR_SPACE_MANUFACTURE_SOURCE, inventory->index.base() + 1));
					else draw(GetString(STR_SPACE_MANUFACTURE_SOURCE_MISSING));
					if (module.manufacture) {
						const auto &job = *module.manufacture;
						if (job.phase == OrbitalManufacturePhase::Queued) draw(GetString(STR_SPACE_MANUFACTURE_WAITING));
						else if (job.phase == OrbitalManufacturePhase::Reserved) draw(GetString(STR_SPACE_MANUFACTURE_RESERVED));
						else if (job.phase == OrbitalManufacturePhase::Complete) draw(GetString(STR_SPACE_MANUFACTURE_COMPLETE, job.product.vehicle.base() + 1));
						else if (job.phase == OrbitalManufacturePhase::AwaitingMaterialization) draw(GetString(STR_SPACE_MANUFACTURE_CAPACITY_WAIT));
						else draw(GetString(STR_SPACE_MANUFACTURE_PROGRESS, job.work_ticks, recipe.work_ticks));
						// Owner-only admission quotes keep the original current company;
						// drawing never borrows another actor's unit/stock permissions.
						if (_current_company == site->state.owner) {
							static constexpr StringID reasons[] = {STR_SPACE_PROCESS_READY, STR_SPACE_PROCESS_INACTIVE,
								STR_SPACE_PROCESS_INPUT, STR_SPACE_PROCESS_POWER, STR_SPACE_PROCESS_OUTPUT};
							draw(GetString(reasons[to_underlying(GetOrbitalManufactureWait(*site))]));
						}
					}
					// Display every declared extra/common input from the canonical recipe; no local two-label table may hide heavy components or imply free output.
					for (size_t input = 0; input < recipe.inputs.size(); ++input) {
						const auto &required = recipe.inputs[input]; const auto cargo = GetCargoTypeByLabel(required.label);
						// A completed row shows the actually embodied material receipt; pending work still displays its original held packets.
						const uint available = module.manufacture ? module.manufacture->inputs[input].TotalCount() + module.manufacture->consumed[input] :
							(inventory != nullptr && inventory->module ? inventory->module->stock[cargo].TotalCount() : 0);
						draw(GetString(STR_SPACE_MANUFACTURE_INPUT, CargoSpec::Get(cargo)->name, available, required.quantity));
					}
				}
				if (module.processing) {
					const auto &state = *module.processing;
					const auto *definition = GetOrbitalRecipeDefinition(site->state.kind, state.choice);
					// Use the saved content choice and its sole original packet/history
					// owners; no local recipe list or displayed quantity authorizes work.
					if (definition != nullptr) {
						const auto &recipe = definition->recipe;
						draw(GetString(STR_SPACE_PROCESS_RECIPE, std::string(state.choice.begin(), state.choice.end())));
						if (_current_company == site->state.owner) {
							static constexpr StringID reasons[] = {STR_SPACE_PROCESS_READY, STR_SPACE_PROCESS_DISABLED,
								STR_SPACE_PROCESS_INACTIVE, STR_SPACE_PROCESS_INPUT, STR_SPACE_PROCESS_OUTPUT,
								STR_SPACE_PROCESS_WORK, STR_SPACE_PROCESS_POWER, STR_SPACE_PROCESS_ALLOCATION, STR_SPACE_PROCESS_LIMIT};
							draw(GetString(reasons[to_underlying(GetOrbitalRecipeWait(*site))]));
						}
						const auto ticks = state.electrical ? state.electrical->WorkTicks() : state.unpowered_work_ticks;
						draw(GetString(STR_SPACE_PROCESS_PROGRESS, ticks, recipe.work_economy_ticks, state.paid_lifetime_j));
						// Each canonical input/output is disclosed even when empty. Missing
						// extra materials or full products cannot disappear behind Steel/Goods.
						for (const auto &input : recipe.inputs) {
							const auto cargo = GetCargoTypeByLabel(input.label);
							if (!IsValidCargoType(cargo)) continue;
							const auto consumed = state.consumed.find(input.label);
							draw(GetString(STR_SPACE_PROCESS_INPUT_STOCK, CargoSpec::Get(cargo)->name,
								module.stock[cargo].TotalCount(), input.quantity, consumed == state.consumed.end() ? 0 : consumed->second));
						}
						for (const auto &output : recipe.outputs) {
							const auto cargo = GetCargoTypeByLabel(output.label);
							if (!IsValidCargoType(cargo)) continue;
							const auto produced = state.produced.find(output.label);
							draw(GetString(STR_SPACE_PROCESS_OUTPUT_STOCK, CargoSpec::Get(cargo)->name,
								module.stock[cargo].TotalCount(), GetOrbitalRecipeCargoCapacity(site->state.kind, output.label),
								produced == state.produced.end() ? 0 : produced->second));
						}
					}
				}
				if (GetOrbitalModuleServiceCapacity(site->state.kind) != 0) {
					const auto &job = *module.GetServiceJobs()[this->service_berth];
					if (site->state.kind == SpaceBuildKind::Dock) {
						draw(GetString(STR_SPACE_DOCK_BERTH, this->service_berth + 1, GetOrbitalModuleServiceCapacity(site->state.kind)));
						if (module.utility_source != SpaceStructureRef{}) draw(GetString(STR_SPACE_DOCK_SOURCE, module.utility_source.structure.base() + 1));
						else if (this->utility_source_selection != SpaceStructureRef{}) draw(GetString(STR_SPACE_DOCK_PROPOSE_SOURCE, this->utility_source_selection.structure.base() + 1));
						else draw(GetString(STR_SPACE_DOCK_SOURCE_MISSING));
						const auto *source = GetOrbitalServicePowerSite(*site);
						draw(GetString(STR_SPACE_CONSTRUCTION_PANEL_OUTPUT, source == nullptr ? 0 : GetOrbitalModulePanelRate(*source) / 1000));
					}
					draw(GetString(STR_CORE_SERVICE_STOCK, module.stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount(), module.stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount()));
					const auto *probe = ResolveSpaceCraft(job.remaining_ticks == 0 ? this->service_craft : job.craft);
					const auto tuning = job.remaining_ticks != 0 ? GetOrbitalServiceJobTuning(job) : (probe == nullptr ? std::nullopt : GetSpaceServiceTuning(GetSpaceVehicleModel(*probe)));
					// A reserved bill belongs to the original saved role even after sender loss; selecting another hull cannot replace its parts or work requirement.
					if (tuning) {
						const auto model = job.remaining_ticks != 0 ? job.model : GetSpaceVehicleModel(*probe);
						draw(GetString(STR_SPACE_SERVICE_BILL, model == UINT16_MAX ? STR_SPACE_SERVICE_LEGACY_ROLE : ManufactureModelName(model), tuning->inputs[0], tuning->inputs[1], tuning->ticks, tuning->returns));
					}
					if (job.remaining_ticks != 0) {
						draw(GetString(STR_SPACE_SERVICE_RESERVED, job.inputs[0].TotalCount(), job.inputs[1].TotalCount()));
						if (probe == nullptr) draw(GetString(STR_SPACE_SERVICE_SENDER_LOST, job.craft.vehicle.base()));
					}
					StringID status = STR_CORE_SERVICE_IDLE;
					// A settled current-epoch receipt remains visible after its grant is spent; polling never renews work or another berth's receipt.
					const bool worked = job.last_work_interval != 0 && job.last_work_interval == GetUtilityPowerInterval();
					if (job.remaining_ticks != 0) status = job.paused ? STR_CORE_SERVICE_PAUSED : (worked || (probe != nullptr && IsOrbitalServiceWorkTick(*probe)) ? STR_CORE_SERVICE_WORKING : STR_CORE_SERVICE_WAITING);
					draw(GetString(STR_CORE_SERVICE_STATUS, status, job.remaining_ticks, module.services_completed));
					if (probe != nullptr) draw(GetString(STR_CORE_SERVICE_CRAFT, probe->index));
				}
			}
		}
		draw(GetString(STR_SPACE_CONSTRUCTION_HINT));
		constexpr StringID phases[] = {STR_SPACE_CONSTRUCTION_BLUEPRINT, STR_SPACE_CONSTRUCTION_FRAME, STR_SPACE_CONSTRUCTION_CANCELLED};
		// Every owned site remains reachable through both the scrolled list and
		// the fixed selector, even when selected details need multiple screens.
		for (const auto *site : SpaceStructure::Iterate()) {
			if (site->state.owner != _local_company) continue;
			const auto ref = GetSpaceStructureReference(*site);
			this->lines.push_back({GetString(STR_SPACE_CONSTRUCTION_SITE, site->index.base() + 1, ModuleName(site->state.kind), site->state.x, site->state.y,
				IsOrbitalModuleOperational(*site) ? STR_SPACE_CONSTRUCTION_READY : phases[to_underlying(site->state.phase)]), ref == this->selected ? TextColour::White : TextColour::Black, ref});
		}
	}

	/** @param pt Window-local pointer position. @param widget Native action identity. @param click_count Framework click count. */
	void OnClick(Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		auto *site = ResolveSpaceStructure(this->selected); auto *v = ResolveSpaceCraft(this->builder);
		// Hit-testing uses the same wrapped pixel bounds as painting and retains
		// the exact incarnation; offscreen rows cannot become click targets.
		if (widget == WID_SC_DETAILS) {
			const int y = pt.y - this->GetWidget<NWidgetBase>(WID_SC_DETAILS)->pos_y - ScaleGUITrad(4) + this->scroll->GetPosition();
			for (const auto &line : this->lines) {
				// Detail rows have no target; only a visible site's own interval selects it.
				if (line.site != SpaceStructureRef{} && y >= line.top && y < line.top + line.height) { this->selected = line.site; break; }
			}
		}
		if (widget == WID_SC_PAGE) {
			DropDownList list;
			// Each fixed category appears once; no hidden command depends on a page
			// being omitted because its current site cannot execute its actions.
			for (uint8_t page = 0; page < to_underlying(SpaceConstructionPage::End); ++page) list.push_back(MakeDropDownListStringItem(PageName(static_cast<SpaceConstructionPage>(page)), page));
			ShowDropDownList(this, std::move(list), to_underlying(this->page), WID_SC_PAGE);
		}
		if (widget == WID_SC_SITE) {
			DropDownList list;
			// Capture each generation alongside the dropdown index so deletion and
			// pool-slot reuse while it is open cannot silently select a new site.
			this->site_choices.clear();
			for (const auto *candidate : SpaceStructure::Iterate()) {
				if (candidate->state.owner != _local_company) continue;
				list.push_back(MakeDropDownListStringItem(GetString(STR_SPACE_CONSTRUCTION_SELECTED, ModuleName(candidate->state.kind), candidate->index.base() + 1), static_cast<int>(this->site_choices.size())));
				this->site_choices.push_back(GetSpaceStructureReference(*candidate));
			}
			ShowDropDownList(this, std::move(list), -1, WID_SC_SITE);
		}
		if (widget == WID_SC_PLACE) {
			const auto &d = *GetSpaceConstructionDefinition(this->kind);
			SetObjectToPlaceWnd(SPR_CURSOR_INDUSTRY, PAL_NONE, HT_RECT, this); SetTileSelectSize(d.width, d.height);
		}
		if (widget == WID_SC_NEXT_BUILDER) this->NextBuilder();
		// Commands revalidate ownership and incarnation; UI availability is only presentation, never mutation authority.
		if (widget == WID_SC_QUEUE && site != nullptr && v != nullptr) Command<Commands::EditSpaceOrder>::Post(STR_SPACE_ORDER_FAILED, v->index, v->generation, SpaceOrderEdit::Insert, static_cast<uint8_t>(v->space_orders.entries.size()), SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, uint32_t{site->index.base()}, site->generation);
		if (widget == WID_SC_CLEAR && v != nullptr) Command<Commands::EditSpaceOrder>::Post(v->index, v->generation, SpaceOrderEdit::Clear, uint8_t{0}, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0});
		// Visiting requires both actual incarnations; a missing selected frame or craft cannot become an implicit destination.
		if (widget == WID_SC_VISIT && site != nullptr && v != nullptr) Command<Commands::EditSpaceOrder>::Post(v->index, v->generation, SpaceOrderEdit::Insert, static_cast<uint8_t>(v->space_orders.entries.size()), SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, uint32_t{site->index.base()}, site->generation);
		// Cancelling addresses only an existing selected blueprint; native admission preserves paid work and actual inputs.
		if (widget == WID_SC_CANCEL && site != nullptr) Command<Commands::CancelSpaceBlueprint>::Post(site->index, site->generation);
		// Returning leftover construction material requires a real frame and carrier; quantity is bounded by its physical remaining hold.
		if ((widget == WID_SC_COLLECT_STEEL || widget == WID_SC_COLLECT_GOODS) && site != nullptr && v != nullptr) {
			const uint slot = widget - WID_SC_COLLECT_STEEL; const auto cargo = GetCargoTypeByLabel(GetSpaceConstructionDefinition(site->state.kind)->inputs[slot].label);
			const auto capacity = GetSpaceFreightCapacity(20, cargo);
			const auto quantity = std::min<uint>(site->job.inputs[slot].TotalCount(), capacity - std::min<uint>(capacity, v->cargo.TotalCount()));
			Command<Commands::CollectSpaceConstructionInputs>::Post(site->index, site->generation, v->index, v->generation, cargo, static_cast<uint16_t>(quantity));
		}
		if (widget == WID_SC_RECIPE) {
			DropDownList list;
			// The authoritative catalogue determines eligible static recipes; mobile hulls and mirrors have no footprint here.
			for (const auto &definition : GetSpaceConstructionDefinitions()) {
				if (definition.width == 0) continue;
				list.push_back(MakeDropDownListStringItem(ModuleName(definition.kind), to_underlying(definition.kind)));
			}
			ShowDropDownList(this, std::move(list), to_underlying(this->kind), WID_SC_RECIPE);
		}
		// Starting activation is an explicit synchronized action on the selected paid frame.
		if (widget == WID_SC_ACTIVATE && site != nullptr) Command<Commands::ActivateOrbitalModule>::Post(site->index, site->generation);
		// These controls move actual common construction cargo at a precise approach; no proximity transfer or payment occurs.
		if (widget >= WID_SC_LOAD_STEEL && widget <= WID_SC_UNLOAD && site != nullptr && site->module != nullptr && v != nullptr) {
			const bool load = widget != WID_SC_UNLOAD;
			const auto cargo = load ? GetCargoTypeByLabel(widget == WID_SC_LOAD_STEEL ? CargoLabel{"STEL"} : CargoLabel{"GOOD"}) : v->cargo_type;
			if (IsValidCargoType(cargo)) {
				const auto capacity = GetSpaceFreightCapacity(20, cargo);
				const uint quantity = load ? std::min<uint>(site->module->stock[cargo].TotalCount(), capacity - std::min<uint>(capacity, v->cargo.TotalCount())) : v->cargo.TotalCount();
				Command<Commands::MoveOrbitalFreight>::Post(site->index, site->generation, v->index, v->generation, cargo, static_cast<uint16_t>(quantity), load);
			}
		}
		// Centering follows the selected native footprint and cannot invent a site from a viewport location.
		if (widget == WID_SC_LOCATE && site != nullptr) ScrollMainWindowToTile(TileXYL(site->state.x, site->state.y, MapLayer::Space));
		if (widget == WID_SC_NEXT_SERVICE) this->NextServiceCraft();
		// Service controls address the saved occupant for existing jobs, preserving cancellation after hull loss.
		if (widget >= WID_SC_START_SERVICE && widget <= WID_SC_CANCEL_SERVICE && site != nullptr && site->module != nullptr) {
			const auto ref = widget == WID_SC_START_SERVICE ? this->service_craft : site->module->GetServiceJobs()[this->service_berth]->craft;
			Command<Commands::ServiceOrbitalProbe>::Post(site->index, site->generation, ref.vehicle, ref.generation, static_cast<OrbitalServiceControl>(widget - WID_SC_START_SERVICE));
		}
		// Panel pause addresses the selected actual module; an unfinished/missing frame cannot publish generation.
		if (widget == WID_SC_TOGGLE_PANEL && site != nullptr && site->module != nullptr) Command<Commands::SetOrbitalPanelPaused>::Post(site->index, site->generation, site->module->power_enabled);
		// Explicit commissioning preserves native paid-frame and original Core admission rather than granting a panel from selection.
		if (widget == WID_SC_COMMISSION && site != nullptr) Command<Commands::CommissionOrbitalServices>::Post(site->index, site->generation);
		// Connections identify both original site generations; selected proposals never create an implicit source or utility account.
		if (widget == WID_SC_CONNECT && site != nullptr) Command<Commands::ConnectOrbitalServices>::Post(site->index, site->generation, this->utility_source_selection.structure, this->utility_source_selection.generation);
		// Only an existing Dock has two selectable service berths; Core and non-service roles cannot acquire a second job from this control.
		if (widget == WID_SC_NEXT_BERTH && site != nullptr && site->module != nullptr && site->state.kind == SpaceBuildKind::Dock) {
			this->service_berth = static_cast<uint8_t>((this->service_berth + 1) % GetOrbitalModuleServiceCapacity(site->state.kind));
			const auto ref = site->module->GetServiceJobs()[this->service_berth]->craft;
			// Lost occupants remain cancellable through the selected saved berth; do not replace that selection with a different live hull.
			if (ResolveSpaceCraft(ref) != nullptr) this->service_craft = ref;
		}
		if (widget == WID_SC_NEXT_SOURCE) this->NextUtilitySource();
		// Each selected-site lifecycle operation revalidates native dependency, inventory and connection ownership before changing anything.
		if (widget == WID_SC_DISCONNECT && site != nullptr) Command<Commands::DisconnectOrbitalServices>::Post(site->index, site->generation);
		// Empty removal belongs to the actual selected incarnation; retained stock or job history cannot disappear through the UI.
		if (widget == WID_SC_REMOVE && site != nullptr) Command<Commands::RemoveOrbitalModule>::Post(site->index, site->generation);
		// Evacuation requires the explicit selected destination and whole native packet admission; choosing it grants no receiving capacity.
		if (widget == WID_SC_EVACUATE && site != nullptr) Command<Commands::EvacuateOrbitalModule>::Post(site->index, site->generation, this->evacuation_destination.structure, this->evacuation_destination.generation);
		if (widget == WID_SC_NEXT_DESTINATION) this->NextEvacuationDestination();
		if (widget == WID_SC_NEXT_MODEL) {
			const std::array<uint16_t, 6> models{15, 19, 20, 16, 17, SPACE_HEAVY_MINER_MODEL};
			const auto current = std::ranges::find(models, this->manufacture_model);
			this->manufacture_model = current == models.end() || std::next(current) == models.end() ? models.front() : *std::next(current);
		}
		if (widget == WID_SC_NEXT_INPUT_SOURCE) this->NextManufactureSource();
		// Queue and cancellation share the exact native packet/funds admission used by tests and later fleet consumers; GUI selection alone grants nothing.
		if (widget == WID_SC_QUEUE_MANUFACTURE && site != nullptr) Command<Commands::QueueOrbitalManufacture>::Post(site->index, site->generation, this->manufacture_source.structure, this->manufacture_source.generation, this->manufacture_model);
		// Cancelling a real selected job retains its paid history and returns only still-unconsumed packets through native admission.
		if (widget == WID_SC_CANCEL_MANUFACTURE && site != nullptr) Command<Commands::CancelOrbitalManufacture>::Post(site->index, site->generation, this->manufacture_source.structure, this->manufacture_source.generation);
		// Only the synchronized command can turn delivered stock into this exact job's exclusive reservation.
		if (widget == WID_SC_RESERVE && site != nullptr) {
			if (const auto *core = GetOrbitalConstructionCore(site->state)) Command<Commands::ReserveOrbitalModuleInputs>::Post(site->index, site->generation, core->index, core->generation);
		}
		// Move one actual service part per click, through the same synchronized precise-exchange command as native tests.
		if (widget >= WID_SC_UNLOAD_METAL && widget <= WID_SC_LOAD_ELECTRONICS && site != nullptr) {
			const auto ref = this->service_craft; const uint8_t supply = static_cast<uint8_t>(1 + (widget - WID_SC_UNLOAD_METAL) % 2);
			Command<Commands::MoveOrbitalSupply>::Post(site->index, site->generation, ref.vehicle, ref.generation, supply, uint16_t{1}, widget >= WID_SC_LOAD_METAL);
		}
		this->RefreshPresentation();
		this->SetDirty();
	}
	/** @param widget Dropdown identity. @param index Canonical selected recipe ordinal. @param button Framework selection button. */
	void OnDropdownSelect(WidgetID widget, int index, [[maybe_unused]] int button) override
	{
		// Dropdown values are presentation choices, not command authority. Refuse
		// stale/out-of-range values before indexing or changing the active page.
		if (widget == WID_SC_PAGE && index >= 0 && index < to_underlying(SpaceConstructionPage::End)) {
			this->page = static_cast<SpaceConstructionPage>(index);
			this->GetWidget<NWidgetStacked>(WID_SC_ACTIONS)->SetDisplayedPlane(index);
			this->ReInit();
			this->RefreshPresentation();
			return;
		}
		// A captured list index is valid only within this dropdown's own identities;
		// another site's recycled native ID must not become a selected target.
		if (widget == WID_SC_SITE && index >= 0 && static_cast<size_t>(index) < this->site_choices.size()) {
			const auto ref = this->site_choices[index];
			if (ResolveSpaceStructure(ref) != nullptr) this->selected = ref;
			this->scroll->SetPosition(0);
			this->RefreshPresentation();
			this->SetDirty();
			return;
		}
		const auto *definition = GetSpaceConstructionDefinition(static_cast<SpaceBuildKind>(index));
		// Only a current static catalogue entry may change the placement recipe and footprint preview.
		if (widget != WID_SC_RECIPE || definition == nullptr || definition->width == 0) return;
		ResetObjectToPlace(); this->kind = definition->kind; this->RefreshPresentation(); this->SetDirty();
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
	window->RefreshPresentation();
	window->SetDirty();
}
