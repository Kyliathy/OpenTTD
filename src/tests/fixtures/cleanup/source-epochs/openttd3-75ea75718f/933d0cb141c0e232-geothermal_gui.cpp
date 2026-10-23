/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_gui.cpp Native-backed source/site choices and synchronized planning/funding in Underground Construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups15CB/15CC/15CDB.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 */
#include "stdafx.h"
#include "geothermal_gui.h"
#include "geothermal_site_cmd.h"
#include "geothermal_utility_cmd.h"
#include "geothermal_service_cmd.h"
#include "geothermal_generator.h"
#include "geothermal_riser.h"
#include "geothermal_collector.h"
#include "geothermal_utility.h"
#include "industry.h"
#include "cargotype.h"
#include "multilayer_sprite.h"
#include "openttd.h"
#include "gfx_func.h"
#include "timer/timer_game_tick.h"
#include "station_base.h"
#include "town.h"
#include "industry_cmd.h"
#include "window_gui.h"
#include "strings_func.h"
#include "dropdown_func.h"
#include "dropdown_type.h"
#include "command_func.h"
#include "company_base.h"
#include "map_layer.h"
#include "tilehighlight_func.h"
#include "core/random_func.hpp"
#include "widgets/underground_widget.h"
#include "table/strings.h"
#include "table/sprites.h"
#include "safeguards.h"

static constexpr std::array _geothermal_site_kinds{MultiLayerGeothermalConstructionKind::Collector,
	MultiLayerGeothermalConstructionKind::Powerhouse, MultiLayerGeothermalConstructionKind::MagmaPowerhouse}; ///< Frozen site families only; native portal risers have a distinct physical command.
static constexpr std::array _geothermal_access_captions{STR_GEO_DIRECTION_NE, STR_GEO_DIRECTION_SE,
	STR_GEO_DIRECTION_SW, STR_GEO_DIRECTION_NW}; ///< Explicit native DiagDirection order, independent of camera or unrelated string-ID ranges.
static constexpr std::array _geothermal_page_captions{STR_GEO_SITES_PAGE, STR_GEO_UTILITIES_PAGE,
	STR_GEO_TOWN_PAGE, STR_GEO_INFORMATION}; ///< Presentation pages keep the existing construction window usable at ordinary screen heights.
static constexpr std::array _geothermal_information_captions{STR_GEO_BUILD_INFORMATION, STR_GEO_ENERGY_INFORMATION,
	STR_GEO_TOWN_PAGE}; ///< Stock, energy and useful export each have their own bounded live information view.
static constexpr std::array _geothermal_page_stacks{WID_UC_GEO_PAGE_STACK, WID_UC_GEO_UTILITIES_STACK,
	WID_UC_GEO_TOWN_STACK, WID_UC_GEO_INFORMATION_STACK}; ///< Independent zero-size stacks avoid reserving the tallest hidden page above actual status text.

/** @param kind Actual frozen selected family. @return Localized player-facing family name; no native definition is allocated. */
static StringID GetGeothermalFamilyCaption(MultiLayerGeothermalConstructionKind kind)
{
	switch (kind) {
		case MultiLayerGeothermalConstructionKind::Collector: return STR_GEO_COLLECTOR;
		case MultiLayerGeothermalConstructionKind::Powerhouse: return STR_GEO_VENT_TAP;
		case MultiLayerGeothermalConstructionKind::MagmaPowerhouse: return STR_GEO_MAGMA;
		case MultiLayerGeothermalConstructionKind::Riser: return STR_GEO_RISER;
		default: return STR_GEO_MENU;
	}
}

/** @param error Actual pure native geometry refusal. @return Business guidance only; commands still revalidate the whole original request. */
static StringID GetGeothermalSiteGuidance(GeothermalError error)
{
	switch (error) {
		case GeothermalError::Unavailable: return STR_GEO_UNAVAILABLE;
		case GeothermalError::InvalidOwner: return STR_GEO_NO_COMPANY;
		case GeothermalError::InvalidVent: return STR_GEO_PICK_VENT;
		case GeothermalError::Footprint: return STR_GEO_FLOOR_REFUSED;
		case GeothermalError::Endpoint: return STR_GEO_PORT_REFUSED;
		case GeothermalError::Occupied:
		case GeothermalError::Claimed: return STR_GEO_OCCUPIED;
		default: return STR_GEO_REFUSED;
	}
}

/** @copydoc UndergroundGeothermalControls::UndergroundGeothermalControls */
UndergroundGeothermalControls::UndergroundGeothermalControls(MapLayer layer) : layer(layer), status(GetString(STR_GEO_HELP)) {}

/** @copydoc UndergroundGeothermalControls::SiteRequest */
GeothermalSiteRequest UndergroundGeothermalControls::SiteRequest() const
{
	return {this->vent, _local_company, this->kind, this->anchor, this->endpoint, this->access};
}

/** @copydoc UndergroundGeothermalControls::SelectedSite */
const GeothermalSiteClaim *UndergroundGeothermalControls::SelectedSite() const
{
	const auto *claim = GetGeothermalSiteClaim(this->site);
	// A dropdown may outlive cancellation, takeover or a different world. Only
	// the actual selected original in this menu's explicit plane is inspected.
	if (claim == nullptr || claim->site.owner != _local_company || !IsValidTile(claim->site.tile) || GetTileLayer(claim->site.tile) != this->layer) return nullptr;
	return claim;
}

/** @copydoc UndergroundGeothermalControls::QuotePlan */
CommandCost UndergroundGeothermalControls::QuotePlan() const
{
	// An incomplete or unavailable cursor request cannot reserve a site or
	// turn presentation state into permission on another company/date/layer.
	if (this->placement != Placement::Site || this->anchor == INVALID_TILE || this->endpoint == INVALID_TILE ||
		!IsGeothermalFacilityToolAvailable(this->kind, this->layer, _local_company)) return CMD_ERROR;
	AutoRestoreBackup actor{_current_company, _local_company};
	return CmdPlanGeothermalSite({}, this->anchor, this->vent, this->kind, this->endpoint, this->access);
}

/** @copydoc UndergroundGeothermalControls::QuoteFunding */
CommandCost UndergroundGeothermalControls::QuoteFunding() const
{
	const auto *claim = this->SelectedSite();
	// Only an actual issued active plan can authorize a capital estimate. A
	// submitted request, consumed plan or remembered coordinate grants nothing.
	if (claim == nullptr || !claim->active || !IsGeothermalFacilityToolAvailable(claim->site.kind, this->layer, _local_company)) return CMD_ERROR;
	AutoRestoreBackup actor{_current_company, _local_company};
	if (claim->site.kind == MultiLayerGeothermalConstructionKind::Collector) return CmdBuildGeothermalCollector({}, claim->site.tile, claim->id, 0);
	return CmdBuildGeothermalGenerator({}, claim->site.tile, claim->id, 0);
}

/** @copydoc UndergroundGeothermalControls::QuoteRiser */
CommandCost UndergroundGeothermalControls::QuoteRiser() const
{
	// Funding requires all three explicit clicks on this page's original rock
	// plane; a source selection or a remembered plan cannot stand in for them.
	if (this->placement != Placement::Riser || this->portal == INVALID_TILE || this->anchor == INVALID_TILE || this->endpoint == INVALID_TILE ||
		!IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::Riser, this->layer, _local_company)) return CMD_ERROR;
	AutoRestoreBackup actor{_current_company, _local_company};
	return CmdBuildGeothermalRiser({}, this->anchor, this->portal, this->endpoint, this->access, 0);
}

/** @copydoc UndergroundGeothermalControls::QuoteService */
CommandCost UndergroundGeothermalControls::QuoteService() const
{
	if (!IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::Riser, this->layer, _local_company)) return CMD_ERROR;
	AutoRestoreBackup actor{_current_company, _local_company};
	return CmdConnectGeothermalTownService({}, this->service.generator, this->service.first_riser,
		this->service.second_riser, this->service.terminal, this->service.town);
}

/** @copydoc UndergroundGeothermalControls::IsSelectedServiceConnected */
bool UndergroundGeothermalControls::IsSelectedServiceConnected() const
{
	if (!IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::Riser, this->layer, _local_company)) return false;
	AutoRestoreBackup actor{_current_company, _local_company};
	const auto *terminal = Station::GetIfValid(this->service.terminal);
	// A retained empty sign or lost original sender still needs repair, even
	// before the next utility tick clears its historical logical binding.
	if (terminal == nullptr || terminal->owner != _local_company || GetTileLayer(terminal->xy) != MapLayer::Surface ||
		(!terminal->facilities.Test(StationFacility::Train) && !terminal->facilities.Test(StationFacility::TruckStop)) ||
		!IsGeothermalRiserOperational(this->service.first_riser)) return false;
	if (this->service.second_riser != IndustryID::Invalid() && !IsGeothermalRiserOperational(this->service.second_riser)) return false; // The optional second actual sender remains mandatory for a two-plane route; a stale edge alone is insufficient.
	const auto port = GetGeothermalGeneratorPort(this->service.generator);
	if (!IsValidTile(port)) return false;
	// An unchanged complete service command refuses duplicate publication.
	// Explain its already connected state only when every selected real native
	// participant and ordered original edge still agrees; never infer a route
	// from coincident housings, a town name or a matching numeric source ID.
	for (const auto &town : GetUtilityPowerConsumers()) {
		if (!town.active || town.owner != _local_company || town.town != this->service.town ||
			town.town_terminal != this->service.terminal || town.town_source_port != port) continue;
		for (const auto &final : GetUtilityPowerLinks()) {
			if (!final.active || final.kind != UtilityLinkKind::ElectricalRiser || final.destination != town.meter.consumer_id) continue; // Only the actual incoming original final crossing can close this service's selected route.
			if (this->service.second_riser == IndustryID::Invalid()) return final.riser == this->service.first_riser && final.source == town.town_supply;
			if (final.riser != this->service.second_riser) continue;
			// A two-riser route retains the original relay identity. Both actual
			// crossings must match rather than treating either leg as generation.
			for (const auto &first : GetUtilityPowerLinks()) {
				if (first.active && first.kind == UtilityLinkKind::ElectricalRiser && first.riser == this->service.first_riser &&
					first.source == town.town_supply && first.destination == final.source) return true;
			}
		}
	}
	return false;
}

/** @copydoc UndergroundGeothermalControls::Refresh */
void UndergroundGeothermalControls::Refresh(Window &window, bool locked) const
{
	const bool available = IsGeothermalFacilityToolAvailable(this->kind, this->layer, _local_company);
	window.SetWidgetDisabledState(WID_UC_GEO_SURVEY, locked);
	window.SetWidgetDisabledState(WID_UC_GEO_PLAN, !available);
	window.SetWidgetDisabledState(WID_UC_GEO_CONFIRM, this->QuotePlan().Failed());
	window.SetWidgetDisabledState(WID_UC_GEO_FUND, this->QuoteFunding().Failed());
	const auto *claim = this->SelectedSite();
	AutoRestoreBackup actor{_current_company, _local_company};
	window.SetWidgetDisabledState(WID_UC_GEO_CANCEL, claim == nullptr || !claim->active || CmdCancelGeothermalSite({}, claim->id).Failed()); // Only a currently owned active cold reservation can be cancelled; actual construction needs its native demolition lifecycle.
	window.SetWidgetLoweredState(WID_UC_GEOTHERMAL, this->expanded);
	const bool physical = IsGeothermalFacilityToolAvailable(MultiLayerGeothermalConstructionKind::Riser, this->layer, _local_company);
	// Heat is Deep-local, while paid electricity also supports the Underground
	// relay. Neither the visible page nor inspection grants construction rights.
	for (const auto widget : {WID_UC_GEO_HEAT, WID_UC_GEO_REMOVE_HEAT}) window.SetWidgetDisabledState(widget, !physical || this->layer != MapLayer::DeepUnderground);
	for (const auto widget : {WID_UC_GEO_ELECTRIC, WID_UC_GEO_REMOVE_ELECTRIC, WID_UC_GEO_RISER}) window.SetWidgetDisabledState(widget, !physical);
	window.SetWidgetDisabledState(WID_UC_GEO_RISER_CONFIRM, this->QuoteRiser().Failed());
	window.SetWidgetDisabledState(WID_UC_GEO_CONNECT, this->QuoteService().Failed());
}

/** @copydoc UndergroundGeothermalControls::OnClick */
bool UndergroundGeothermalControls::OnClick(Window &window, WidgetID widget)
{
	if (widget < WID_UC_GEOTHERMAL || widget > WID_UC_GEO_STATUS) return false; // The other components retain their own disjoint widget/input families.
	if (window.IsWidgetDisabled(widget)) return true;
	switch (widget) {
		case WID_UC_GEO_PAGE: {
			DropDownList list;
			for (uint index = 0; index < _geothermal_page_captions.size(); ++index) list.push_back(MakeDropDownListStringItem(GetString(_geothermal_page_captions[index]), index));
			ShowDropDownList(&window, std::move(list), to_underlying(this->page), widget); break;
		}
		case WID_UC_GEO_INFORMATION: {
			DropDownList list;
			for (uint index = 0; index < _geothermal_information_captions.size(); ++index) list.push_back(MakeDropDownListStringItem(GetString(_geothermal_information_captions[index]), index));
			ShowDropDownList(&window, std::move(list), to_underlying(this->information), widget); break;
		}
		case WID_UC_GEO_HEAT:
		case WID_UC_GEO_ELECTRIC:
		case WID_UC_GEO_REMOVE_HEAT:
		case WID_UC_GEO_REMOVE_ELECTRIC:
		case WID_UC_GEO_RISER:
			ResetObjectToPlace();
			this->placement = widget == WID_UC_GEO_RISER ? Placement::Riser : Placement::Utility;
			this->domain = widget == WID_UC_GEO_HEAT || widget == WID_UC_GEO_REMOVE_HEAT ? UtilityEnergyDomain::Heat : UtilityEnergyDomain::Electricity; // The chosen physical action names its domain; matching numeric accounts never imply a connection.
			this->remove_cell = widget == WID_UC_GEO_REMOVE_HEAT || widget == WID_UC_GEO_REMOVE_ELECTRIC; // Removal is explicit and never inferred from occupancy, preventing accidental destruction of an installed route.
			SetObjectToPlaceWnd(SPR_CURSOR_QUERY, PAL_NONE, HT_RECT, &window);
			this->status = GetString(this->placement == Placement::Riser ? STR_GEO_RISER_PORTAL_PROMPT : STR_GEO_CELL_PROMPT);
			break;
		case WID_UC_GEO_RISER_CONFIRM:
			if (this->QuoteRiser().Failed()) break;
			if (Command<Commands::BuildGeothermalRiser>::Post(STR_ERROR_GEO_ACTION, this->anchor, this->portal, this->endpoint, this->access, InteractiveRandom())) {
				this->inspected = this->anchor; ResetObjectToPlace(); this->status = GetString(STR_GEO_SUBMITTED);
			}
			break;
		case WID_UC_GEO_CONNECT:
			if (this->QuoteService().Failed()) { this->status = GetString(STR_GEO_EXPORT_REFUSED); break; }
			Command<Commands::ConnectGeothermalTownService>::Post(STR_ERROR_GEO_ACTION,
				this->service.generator, this->service.first_riser, this->service.second_riser, this->service.terminal, this->service.town);
			this->status = GetString(STR_GEO_SUBMITTED); break;
		case WID_UC_GEO_GENERATOR: {
			DropDownList list;
			// Native completed owned machines provide choices; their actual
			// current ports/output are revalidated by the complete service quote.
			for (const auto &state : CaptureGeothermalGenerators()) {
				if (state.owner != _local_company || !IsGeothermalGeneratorConstructionComplete(state.industry)) continue; // Foreign or unfinished machinery cannot be offered as an operating source.
				const auto tile = GetGeothermalGeneratorPort(state.industry);
				if (!IsValidTile(tile)) continue;
				list.push_back(MakeDropDownListStringItem(GetString(STR_GEO_GENERATOR_CHOICE, GetString(state.model == GeothermalGeneratorModel::VentTap ? STR_GEO_VENT_TAP : STR_GEO_MAGMA), TileX(tile), TilePlaneY(tile)), state.industry.base()));
			}
			if (list.empty()) { this->status = GetString(STR_GEO_NO_GENERATORS); break; }
			ShowDropDownList(&window, std::move(list), this->service.generator.base(), widget); break;
		}
		case WID_UC_GEO_FIRST_RISER:
		case WID_UC_GEO_SECOND_RISER: {
			DropDownList list;
			const bool second = widget == WID_UC_GEO_SECOND_RISER;
			if (second) list.push_back(MakeDropDownListStringItem(GetString(STR_GEO_DIRECT_ROUTE), 0));
			// Route order uses the actual original sending plane. No account or
			// implicit coincident housing is exposed as a routing choice.
			for (const auto &state : CaptureGeothermalRisers()) {
				if (state.site.owner != _local_company || !IsGeothermalRiserOperational(state.industry) || GetTileLayer(state.site.tile) != (second ? MapLayer::Underground : MapLayer::DeepUnderground)) continue; // Only completed owned senders on the requested leg can form this explicit physical route.
				list.push_back(MakeDropDownListStringItem(GetString(STR_GEO_GENERATOR_CHOICE, GetString(STR_GEO_RISER), TileX(state.site.tile), TilePlaneY(state.site.tile)), state.industry.base() + 1));
			}
			if (list.empty()) { this->status = GetString(STR_GEO_NO_RISERS); break; }
			const auto id = second ? this->service.second_riser : this->service.first_riser;
			ShowDropDownList(&window, std::move(list), id == IndustryID::Invalid() ? 0 : id.base() + 1, widget); break;
		}
		case WID_UC_GEO_TERMINAL: {
			DropDownList list;
			// List real owned Surface freight terminals. The atomic provider
			// additionally checks occupied serving tiles and town authority.
			for (const Station *station : Station::Iterate()) {
				if (station->owner != _local_company || GetTileLayer(station->xy) != MapLayer::Surface ||
					(!station->facilities.Test(StationFacility::Train) && !station->facilities.Test(StationFacility::TruckStop))) continue; // An empty sign, passenger-only stop or other-plane station cannot receive this paid physical service.
				list.push_back(MakeDropDownListStringItem(GetString(STR_GEO_TERMINAL_CHOICE, TileX(station->xy), TilePlaneY(station->xy)), station->index.base()));
			}
			if (list.empty()) { this->status = GetString(STR_GEO_NO_TERMINALS); break; }
			ShowDropDownList(&window, std::move(list), this->service.terminal.base(), widget); break;
		}
		case WID_UC_GEO_TOWN: {
			DropDownList list;
			// Towns retain their native identities and single service effect;
			// choosing a name neither authorizes it nor creates a power consumer.
			for (const Town *town : Town::Iterate()) list.push_back(MakeDropDownListStringItem(GetString(STR_GEO_TOWN_CHOICE, town->index), town->index.base()));
			if (list.empty()) { this->status = GetString(STR_GEO_NO_TOWNS); break; }
			ShowDropDownList(&window, std::move(list), this->service.town.base(), widget); break;
		}
		case WID_UC_GEOTHERMAL:
			this->expanded = !this->expanded;
			if (!this->expanded && this->IsPlacing()) ResetObjectToPlace(); // An invisible component cannot retain a cursor or an uncommitted geometry draft.
			window.GetWidget<NWidgetStacked>(WID_UC_GEO_STACK)->SetDisplayedPlane(this->expanded ? 0 : SZSP_NONE);
			window.ReInit();
			break;
		case WID_UC_GEO_SURVEY:
		case WID_UC_GEO_PLAN:
			ResetObjectToPlace(); // The actual parent abort callback cancels any previous tube/excavation/geothermal placement first.
			this->placement = widget == WID_UC_GEO_SURVEY ? Placement::Survey : Placement::Site;
			this->input_refused = false;
			if (this->placement == Placement::Site) this->site = GeothermalSiteID{UINT32_MAX};
			SetObjectToPlaceWnd(SPR_CURSOR_QUERY, PAL_NONE, HT_RECT, &window);
			this->status = GetString(widget == WID_UC_GEO_SURVEY ? STR_GEO_INSPECT_PROMPT : STR_GEO_ANCHOR_PROMPT);
			if (this->placement == Placement::Survey) MarkWholeScreenDirty(); // The full natural field overlay must appear immediately, including outside the one current cursor rectangle.
			break;
		case WID_UC_GEO_CONFIRM:
			if (this->QuotePlan().Failed()) { this->status = GetString(STR_GEO_REFUSED); break; }
			if (Command<Commands::PlanGeothermalSite>::Post(STR_ERROR_GEO_ACTION, this->anchor, this->vent, this->kind, this->endpoint, this->access)) {
				ResetObjectToPlace(); this->status = GetString(STR_GEO_SUBMITTED);
			}
			break;
		case WID_UC_GEO_FUND: {
			if (this->QuoteFunding().Failed()) { this->status = GetString(STR_GEO_FUND_REFUSED); break; }
			const auto *claim = this->SelectedSite(); if (claim == nullptr) break;
			const auto tile = claim->site.tile;
			const auto id = claim->id;
			// The synchronized creation seed belongs to this deliberate player
			// action, never painting, a quote or a simulation-owned RNG advance.
			if (claim->site.kind == MultiLayerGeothermalConstructionKind::Collector) {
				Command<Commands::BuildGeothermalCollector>::Post(STR_ERROR_GEO_ACTION, tile, id, InteractiveRandom());
			} else {
				Command<Commands::BuildGeothermalGenerator>::Post(STR_ERROR_GEO_ACTION, tile, id, InteractiveRandom());
			}
			this->inspected = tile; this->status = GetString(STR_GEO_SUBMITTED);
			break;
		}
		case WID_UC_GEO_CANCEL:
			if (const auto *claim = this->SelectedSite()) Command<Commands::CancelGeothermalSite>::Post(STR_ERROR_GEO_ACTION, claim->id);
			break;
		case WID_UC_GEO_FAMILY: {
			DropDownList list;
			// All three real dated families remain inspectable; selecting a
			// future name cannot enable its native planning/funding commands.
			for (const auto kind : _geothermal_site_kinds) {
				const auto *terms = GetMultiLayerGeothermalConstruction(kind);
				list.push_back(MakeDropDownListStringItem(GetString(STR_GEO_FAMILY_CHOICE, GetString(GetGeothermalFamilyCaption(kind)), terms->introduction_year.base()), to_underlying(kind)));
			}
			ShowDropDownList(&window, std::move(list), to_underlying(this->kind), widget);
			break;
		}
		case WID_UC_GEO_VENT: {
			DropDownList list;
			// This is the existing public natural survey. Menu rows retain only
			// original IDs, without new discovery flags or copied heat budgets.
			for (const auto &vent : GetGeologyWorldState().vents) {
				list.push_back(MakeDropDownListStringItem(GetString(STR_GEO_VENT_CHOICE, vent.x, vent.y, vent.thermal_capacity_w), vent.id.base()));
			}
			if (list.empty()) { this->status = GetString(STR_GEO_NO_VENTS); break; }
			ShowDropDownList(&window, std::move(list), this->vent.base(), widget);
			break;
		}
		case WID_UC_GEO_SITE: {
			DropDownList list;
			// Rebuild the bounded list from actual issued owned active claims;
			// asynchronous planning never predicts or fabricates a funding ID.
			for (const auto &claim : CaptureGeothermalSiteClaims()) {
				if (!claim.active || claim.site.owner != _local_company || GetTileLayer(claim.site.tile) != this->layer) continue; // Cancelled, consumed, foreign and other-plane plans cannot be offered as fundable sites.
				list.push_back(MakeDropDownListStringItem(GetString(STR_GEO_SITE_CHOICE, GetString(GetGeothermalFamilyCaption(claim.site.kind)), TileX(claim.site.tile), TilePlaneY(claim.site.tile)), claim.id.base()));
			}
			if (list.empty()) { this->status = GetString(STR_GEO_NO_PLANS); break; }
			ShowDropDownList(&window, std::move(list), this->site.base(), widget);
			break;
		}
		case WID_UC_GEO_ACCESS: {
			DropDownList list;
			// Direction is an explicit part of the cold blueprint, independent
			// of the camera and of any later selected machinery art.
			for (uint direction = 0; direction < 4; ++direction) {
				list.push_back(MakeDropDownListStringItem(GetString(STR_GEO_ACCESS_CHOICE, GetString(_geothermal_access_captions[direction])), direction));
			}
			ShowDropDownList(&window, std::move(list), to_underlying(this->access), widget);
			break;
		}
		default: break;
	}
	window.SetDirty(); return true;
}

/** @copydoc UndergroundGeothermalControls::OnDropdownSelect */
bool UndergroundGeothermalControls::OnDropdownSelect(Window &window, WidgetID widget, int index)
{
	if (widget < WID_UC_GEOTHERMAL || widget > WID_UC_GEO_STATUS) return false; // Another component's response cannot replace this request or selection.
	if (index < 0) return true;
	switch (widget) {
		case WID_UC_GEO_PAGE:
			if (static_cast<uint>(index) >= _geothermal_page_captions.size()) break;
			if (this->IsPlacing()) ResetObjectToPlace();
			this->page = static_cast<Page>(index);
			// Normal stacked planes deliberately share a maximum height. These
			// independent zero-size stacks instead reclaim every hidden page row,
			// keeping native information adjacent to its visible selector.
			for (uint page = 0; page < _geothermal_page_stacks.size(); ++page) window.GetWidget<NWidgetStacked>(_geothermal_page_stacks[page])->SetDisplayedPlane(page == static_cast<uint>(index) ? 0 : SZSP_NONE);
			window.GetWidget<NWidgetStacked>(WID_UC_GEO_ACCESS_STACK)->SetDisplayedPlane(index < 2 ? 0 : SZSP_NONE);
			this->status = GetString(this->page == Page::Town ? STR_GEO_EXPORT_PROMPT : STR_GEO_HELP);
			window.ReInit(); break;
		case WID_UC_GEO_INFORMATION:
			if (static_cast<uint>(index) < _geothermal_information_captions.size()) this->information = static_cast<Information>(index);
			break;
		case WID_UC_GEO_GENERATOR:
			if (index >= UINT16_MAX) break; // Native pool identities cannot wrap from a delayed or malformed dropdown value.
			this->service.generator = IndustryID{static_cast<uint16_t>(index)};
			this->inspected = GetGeothermalGeneratorPort(this->service.generator); break;
		case WID_UC_GEO_FIRST_RISER:
		case WID_UC_GEO_SECOND_RISER: {
			if (index > UINT16_MAX) break; // Zero means the explicit direct route; remaining rows encode the real native identity plus one.
			const auto id = index == 0 ? IndustryID::Invalid() : IndustryID{static_cast<uint16_t>(index - 1)};
			if (widget == WID_UC_GEO_FIRST_RISER) this->service.first_riser = id; else this->service.second_riser = id;
			break;
		}
		case WID_UC_GEO_TERMINAL:
			if (index >= UINT16_MAX) break; // Preserve native identity width before looking up an actual current terminal.
			this->service.terminal = StationID{static_cast<uint16_t>(index)}; break;
		case WID_UC_GEO_TOWN:
			if (index >= UINT16_MAX) break; // A large stale menu response cannot select a different valid town after truncation.
			this->service.town = TownID{static_cast<uint16_t>(index)}; break;
		case WID_UC_GEO_FAMILY: {
			if (index > UINT8_MAX) break; // A delayed/invalid menu value cannot wrap into a different valid immutable family.
			const auto kind = static_cast<MultiLayerGeothermalConstructionKind>(index);
			if (std::ranges::find(_geothermal_site_kinds, kind) == _geothermal_site_kinds.end()) break;
			if (this->IsPlacing()) ResetObjectToPlace();
			this->kind = kind; break;
		}
		case WID_UC_GEO_VENT: {
			const auto id = GeothermalVentID{static_cast<uint32_t>(index)};
			const auto &vents = GetGeologyWorldState().vents;
			if (std::ranges::find(vents, id, &GeothermalVentField::id) != vents.end()) this->vent = id;
			break;
		}
		case WID_UC_GEO_SITE: {
			if (this->IsPlacing()) ResetObjectToPlace();
			this->site = GeothermalSiteID{static_cast<uint32_t>(index)};
			if (const auto *claim = this->SelectedSite()) { this->kind = claim->site.kind; this->vent = claim->site.vent; this->access = claim->site.access; this->inspected = claim->site.tile; }
			break;
		}
		case WID_UC_GEO_ACCESS:
			if (index < 4) this->access = static_cast<DiagDirection>(index);
			break;
		default: return false;
	}
	window.SetDirty(); return true;
}

/** @copydoc UndergroundGeothermalControls::OnPlaceObject */
bool UndergroundGeothermalControls::OnPlaceObject(Window &window, TileIndex tile)
{
	if (!this->IsPlacing()) return false;
	// Cursor ownership is explicit. An extra/main viewport change cannot
	// redirect this draft onto a coincident tile in another plane.
	if (!IsValidTile(tile) || GetTileLayer(tile) != this->layer) {
		this->input_refused = true;
		this->status = GetString(STR_UNDERGROUND_WRONG_LAYER);
	} else if (this->placement == Placement::Survey) {
		this->input_refused = false;
		this->inspected = tile;
		const auto actual = GetGeothermalControlInspection(tile);
		if (actual.vent != nullptr) this->vent = actual.vent->id;
		if (actual.site != nullptr) this->site = actual.site->id;
		this->status = GetString(actual.vent == nullptr ? STR_GEO_NO_VENT_HERE : STR_GEO_SURVEY_READY);
	} else if (this->placement == Placement::Utility) {
		this->input_refused = false;
		AutoRestoreBackup actor{_current_company, _local_company};
		const auto quote = this->remove_cell ? CmdRemoveGeothermalUtilityLink({}, tile, this->domain) : CmdBuildGeothermalUtilityLink({}, tile, this->domain);
		if (quote.Failed()) { this->status = GetString(STR_GEO_CELL_REFUSED); }
		else {
			// The one clicked original domain cell uses the ordinary synchronized
			// paid command. Inspection never creates an installed link or energy.
			if (this->remove_cell) Command<Commands::RemoveGeothermalUtilityLink>::Post(STR_ERROR_GEO_ACTION, tile, this->domain);
			else Command<Commands::BuildGeothermalUtilityLink>::Post(STR_ERROR_GEO_ACTION, tile, this->domain);
			this->inspected = tile; this->status = GetString(STR_GEO_SUBMITTED);
		}
	} else if (this->placement == Placement::Riser && this->portal == INVALID_TILE) {
		// Riser funding later checks this exact original commissioned sender
		// and reciprocal incarnation; this click alone acquires no attachment.
		this->input_refused = false; this->portal = tile; this->status = GetString(STR_GEO_RISER_ANCHOR_PROMPT);
	} else if (this->anchor == INVALID_TILE) {
		this->input_refused = false;
		this->anchor = tile; this->status = GetString(STR_GEO_PORT_PROMPT);
	} else {
		this->input_refused = false;
		this->endpoint = tile;
		AutoRestoreBackup actor{_current_company, _local_company};
		if (this->placement == Placement::Riser) this->status = GetString(this->QuoteRiser().Succeeded() ? STR_GEO_RISER_READY : STR_GEO_RISER_REFUSED);
		else {
			const auto admitted = PrepareGeothermalSite(this->SiteRequest());
			this->status = GetString(this->QuotePlan().Succeeded() ? STR_GEO_PLAN_READY : GetGeothermalSiteGuidance(admitted.error));
		}
	}
	window.SetDirty(); return true;
}

/** @copydoc UndergroundGeothermalControls::Abort */
void UndergroundGeothermalControls::Abort()
{
	this->placement = Placement::None; this->input_refused = false; this->portal = this->anchor = this->endpoint = INVALID_TILE;
}

/** @copydoc UndergroundGeothermalControls::GetSurveySprite */
std::optional<SpriteID> UndergroundGeothermalControls::GetSurveySprite(TileIndex tile) const
{
	// Only this armed visible survey may reveal a natural original in its own
	// unlocked rock plane; a retained window/selection or camera supplies none.
	if (!this->expanded || this->placement != Placement::Survey || !IsValidTile(tile) || GetTileLayer(tile) != this->layer ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		!CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return std::nullopt;
	const auto &geology = GetGeologyWorldState();
	if (FindGeothermalVentField(geology, tile) == nullptr) return std::nullopt;
	const auto floor = geology.terrain.Get(tile);
	if (!floor.has_value()) return std::nullopt;
	const uint8_t state = *floor == ExcavationState::Excavated ? 2 : 1;
	return GetMultiLayerSprite(MultiLayerSpriteAsset::GeothermalVent,
		{state, GetMultiLayerVisualFrame(MultiLayerSpriteAsset::GeothermalVent, TimerGameTick::counter, false)});
}

/** @copydoc UndergroundGeothermalControls::GetInformationText */
std::string UndergroundGeothermalControls::GetInformationText() const
{
	AutoRestoreBackup actor{_current_company, _local_company};
	if (this->information == Information::Town) {
		if (!Town::IsValidID(this->service.town)) return GetString(STR_GEO_EXPORT_PROMPT);
		// Borrow the original town account, including a temporarily broken
		// terminal binding. Inspection never registers or repairs a service.
		for (const auto &consumer : GetUtilityPowerConsumers()) {
			if (!consumer.active || consumer.owner != _local_company || consumer.town != this->service.town) continue; // Retired, foreign or other-town histories cannot be presented as this selected service.
			uint64_t used = 0;
			for (const auto &share : consumer.shares) used += share.consumed_j; // These are the original current-epoch final-town debits; transferred upstream slices are never counted as town use.
			const auto remaining = GetUtilityAllocation(consumer.meter.consumer_id);
			std::string text = GetString(STR_GEO_TOWN_CHOICE, consumer.town);
			text += "\n" + GetString(STR_GEO_SERVICE_METER, used, GetUtilityTownPowerBonus(consumer.town));
			text += "\n" + GetString(STR_GEO_GRANT_METER, consumer.meter.demand_energy_j, consumer.meter.granted_energy_j,
				remaining.has_value() ? remaining->granted_energy_j : 0);
			// Show only the original final crossing's current metered output.
			// Summing both riser legs would count the same exported joules twice.
			for (const auto &link : GetUtilityPowerLinks()) {
				if (!link.active || link.kind != UtilityLinkKind::ElectricalRiser || link.destination != consumer.meter.consumer_id) continue; // Only the real active incoming town leg measures delivered export; local cells and upstream relay flow are different observations.
				text += "\n" + GetString(STR_GEO_CURRENT_ELECTRIC, link.flow_j); break;
			}
			if (consumer.town_terminal == StationID::Invalid()) text += "\n" + GetString(STR_GEO_WAITING, GetString(STR_GEO_TERMINAL_LOST));
			else if (consumer.town_effect_suspended) text += "\n" + GetString(STR_GEO_WAITING, GetString(STR_GEO_TOWN_SUSPENDED));
			else if (!consumer.town_effect_applied) text += "\n" + GetString(STR_GEO_WAITING, GetString(STR_GEO_NEXT_INTERVAL));
			return text;
		}
		return GetString(STR_GEO_EXPORT_PROMPT);
	}
	const auto actual = GetGeothermalControlInspection(this->inspected);
	if (actual.industry == nullptr) {
		if (actual.vent != nullptr) return GetString(STR_GEO_SURVEY_CAPACITY, actual.vent->thermal_capacity_w);
		return GetString(STR_GEO_INSPECT_PROMPT);
	}
	const auto kind = actual.collector != nullptr ? MultiLayerGeothermalConstructionKind::Collector :
		actual.riser != nullptr ? MultiLayerGeothermalConstructionKind::Riser : GetMultiLayerGeothermalGeneratorTerms(actual.generator->model)->construction;
	const auto *terms = GetMultiLayerGeothermalConstruction(kind);
	const auto owner = actual.collector != nullptr ? actual.collector->owner : actual.riser != nullptr ? actual.riser->site.owner : actual.generator->owner;
	const auto work = actual.collector != nullptr ? actual.collector->work_ticks : actual.riser != nullptr ? actual.riser->work_ticks : actual.generator->work_ticks;
	const bool committed = actual.collector != nullptr ? actual.collector->materials_committed : actual.riser != nullptr ? actual.riser->materials_committed : actual.generator->materials_committed;
	bool has_bill = true;
	// Stock readiness is an inspection of every original native accepted slot,
	// distinct from the later whole-bill consumption/work receipt.
	for (const auto &input : terms->inputs) {
		const auto slot = std::ranges::find(actual.industry->accepted, GetCargoTypeByLabel(input.label), &Industry::AcceptedCargo::cargo);
		if (slot == actual.industry->accepted.end() || slot->waiting < input.quantity) { has_bill = false; break; } // Any actual missing bill component still blocks native construction; surplus in another slot cannot replace it.
	}
	std::string text = GetString(GetGeothermalFamilyCaption(kind));
	if (this->information == Information::Construction) {
		text += "\n" + GetString(STR_GEO_CONSTRUCTION, uint32_t{work} * 100 / terms->work_ticks);
		text += "\n" + GetString(committed ? STR_GEO_BILL_CONSUMED : has_bill ? STR_GEO_BILL_READY : STR_GEO_BILL_REQUIRED);
		// Each frozen bill row resolves the actual native accepted slot. A
		// committed bill remains historical consumption, never available stock.
		for (const auto &input : terms->inputs) {
			const auto cargo = GetCargoTypeByLabel(input.label);
			const auto slot = std::ranges::find(actual.industry->accepted, cargo, &Industry::AcceptedCargo::cargo);
			text += "\n" + GetString(STR_GEO_STOCK, GetString(CargoSpec::Get(cargo)->name), slot == actual.industry->accepted.end() ? 0 : slot->waiting, input.quantity);
		}
	} else {
		if (actual.vent != nullptr) text += "\n" + GetString(STR_GEO_SURVEY_CAPACITY, actual.vent->thermal_capacity_w);
		if (actual.heat != nullptr) {
			text += "\n" + GetString(STR_GEO_SOURCE_METER, actual.heat->meter.generation_energy_j, actual.heat->reserved_j);
			text += "\n" + GetString(STR_GEO_SOURCE_LIFETIME, actual.heat->generated_lifetime_j, actual.heat->consumed_lifetime_j);
		}
		if (actual.heat_input != nullptr) {
			const auto remaining = GetUtilityHeatAllocation(actual.heat_input->meter.consumer_id);
			text += "\n" + GetString(STR_GEO_GRANT_METER, actual.heat_input->meter.demand_energy_j, actual.heat_input->meter.granted_energy_j,
				remaining.has_value() ? remaining->granted_energy_j : 0);
		}
		if (actual.generator != nullptr) text += "\n" + GetString(STR_GEO_CONVERSION, GetMultiLayerGeothermalGeneratorTerms(actual.generator->model)->efficiency_percent);
		if (actual.electricity != nullptr) {
			text += "\n" + GetString(STR_GEO_CURRENT_ELECTRIC, actual.electricity->meter.generation_energy_j);
			text += "\n" + GetString(STR_GEO_ELECTRIC_METER, actual.electricity->generated_lifetime_j, actual.electricity->consumed_lifetime_j);
		}
	}
	// Report actual native admission/shortage, in dependency order. Rendering
	// never quotes a paused operating commit or allocates another opportunity.
	StringID blocked = STR_GEO_NEXT_INTERVAL;
	if (!IsGeothermalFacilityToolAvailable(kind, this->layer, owner)) blocked = STR_GEO_UNAVAILABLE;
	else if (!committed && !has_bill) blocked = STR_GEO_BILL_REQUIRED; // Only an actually incomplete unconsumed bill needs more delivery; consumed historical stock is never demanded again.
	else if (!committed || work < terms->work_ticks) blocked = STR_GEO_LABOUR_REQUIRED; // A full delivered bill still requires normal native paid labour, including the first opportunity which consumes it.
	else if (actual.riser != nullptr && !IsGeothermalRiserOperational(actual.industry->index)) blocked = STR_GEO_RISER_REFUSED; // Fully built machinery still needs its original live commissioned attachment; work alone cannot revive a lost portal.
	else if (actual.generator != nullptr && !HasGeothermalGeneratorInstalledPorts(actual.industry->index)) blocked = STR_GEO_PORTS_REQUIRED; // Completed structure creates no implicit Heat/Electric cells at its original port.
	else if (actual.generator != nullptr) {
		const auto *source = actual.vent == nullptr ? nullptr : GetGeothermalCollectorState(FindGeothermalCollectorForVent(actual.vent->id));
		if (source == nullptr || !IsGeothermalCollectorOperational(source->industry)) blocked = STR_GEO_SOURCE_REQUIRED; // Original natural capacity is no operating offer until its actual owned collector is ready.
		else {
			const auto *original = GetGeothermalSiteClaim(source->site);
			if (original == nullptr || !FindGeothermalUtilityPath(owner, UtilityEnergyDomain::Heat, original->site.endpoint, GetGeothermalGeneratorPort(actual.industry->index)).has_value()) blocked = STR_GEO_HEAT_PATH_REQUIRED; // Real installed same-domain connectivity is required even when both endpoint cells survive.
			else if (!HasGeothermalGeneratorServiceFunds(actual.industry->index, 1)) blocked = STR_GEO_FUNDS_REQUIRED;
			else if (actual.heat_input != nullptr && actual.heat_input->meter.unmet_energy_j != 0) blocked = STR_GEO_HEAT_SHORTAGE; // Only the native allocator's actual unmet demand establishes this shortage; no guessed rate is treated as a grant.
		}
	}
	if (blocked == STR_GEO_NEXT_INTERVAL && _pause_mode.Any()) blocked = STR_GEO_PAUSED; // A generic next-opportunity wait can identify pause; a specific native shortage remains visible while paused.
	text += "\n" + GetString(STR_GEO_WAITING, GetString(blocked));
	return text;
}

/** @copydoc UndergroundGeothermalControls::GetWidgetText */
std::optional<std::string> UndergroundGeothermalControls::GetWidgetText(WidgetID widget) const
{
	switch (widget) {
		case WID_UC_GEO_PAGE: return GetString(_geothermal_page_captions[to_underlying(this->page)]);
		case WID_UC_GEO_INFORMATION: return GetString(_geothermal_information_captions[to_underlying(this->information)]);
		case WID_UC_GEO_GENERATOR: {
			const auto *state = GetGeothermalGeneratorState(this->service.generator);
			if (state == nullptr) return GetString(STR_GEO_SELECT_GENERATOR);
			const auto tile = GetGeothermalGeneratorPort(state->industry);
			if (!IsValidTile(tile)) return GetString(STR_GEO_SELECT_GENERATOR);
			return GetString(STR_GEO_GENERATOR_CHOICE, GetString(state->model == GeothermalGeneratorModel::VentTap ? STR_GEO_VENT_TAP : STR_GEO_MAGMA), TileX(tile), TilePlaneY(tile));
		}
		case WID_UC_GEO_FIRST_RISER:
		case WID_UC_GEO_SECOND_RISER: {
			const bool second = widget == WID_UC_GEO_SECOND_RISER;
			const auto *state = GetGeothermalRiserState(second ? this->service.second_riser : this->service.first_riser);
			if (state == nullptr) return GetString(second ? STR_GEO_DIRECT_ROUTE : STR_GEO_SELECT_FIRST_RISER);
			return GetString(STR_GEO_GENERATOR_CHOICE, GetString(STR_GEO_RISER), TileX(state->site.tile), TilePlaneY(state->site.tile));
		}
		case WID_UC_GEO_TERMINAL:
			if (const auto *station = Station::GetIfValid(this->service.terminal)) return GetString(STR_GEO_TERMINAL_CHOICE, TileX(station->xy), TilePlaneY(station->xy));
			return GetString(STR_GEO_SELECT_TERMINAL);
		case WID_UC_GEO_TOWN:
			if (Town::IsValidID(this->service.town)) return GetString(STR_GEO_TOWN_CHOICE, this->service.town);
			return GetString(STR_GEO_SELECT_TOWN);
		case WID_UC_GEO_FAMILY: return GetString(GetGeothermalFamilyCaption(this->kind));
		case WID_UC_GEO_ACCESS: return GetString(STR_GEO_ACCESS_CHOICE, GetString(_geothermal_access_captions[to_underlying(this->access)]));
		case WID_UC_GEO_VENT: {
			const auto &vents = GetGeologyWorldState().vents;
			const auto vent = std::ranges::find(vents, this->vent, &GeothermalVentField::id);
			if (vent == vents.end()) return GetString(STR_GEO_PICK_VENT);
			return GetString(STR_GEO_VENT_CHOICE, vent->x, vent->y, vent->thermal_capacity_w);
		}
		case WID_UC_GEO_SITE:
			if (const auto *claim = this->SelectedSite()) return GetString(STR_GEO_SITE_CHOICE,
				GetString(GetGeothermalFamilyCaption(claim->site.kind)), TileX(claim->site.tile), TilePlaneY(claim->site.tile));
			return GetString(STR_GEO_SELECT_PLAN);
		case WID_UC_GEO_STATUS: {
			if (this->page == Page::Information) return this->GetInformationText();
			std::string text = this->status;
			// A quote label follows actual current permission/geometry, rather
			// than retaining a Ready message after date, source or floor loss.
			// Keep an explicit wrong-plane click warning until valid input arrives.
			if (this->placement == Placement::Site && !this->input_refused && this->anchor != INVALID_TILE && this->endpoint != INVALID_TILE) {
				AutoRestoreBackup actor{_current_company, _local_company};
				const auto admitted = PrepareGeothermalSite(this->SiteRequest());
				text = GetString(this->QuotePlan().Succeeded() ? STR_GEO_PLAN_READY : GetGeothermalSiteGuidance(admitted.error));
			}
			// Re-query the whole paid machinery quote after access/date/portal
			// loss, while preserving a last explicit wrong-plane click warning.
			if (this->placement == Placement::Riser && !this->input_refused && this->endpoint != INVALID_TILE) {
				const auto quote = this->QuoteRiser();
				text = GetString(quote.Succeeded() ? STR_GEO_RISER_READY : STR_GEO_RISER_REFUSED);
				if (quote.Succeeded()) text += "\n" + GetString(STR_GEO_FUND_QUOTE, quote.GetCost());
			}
			if (this->page == Page::Town) text = GetString(this->QuoteService().Succeeded() ? STR_GEO_EXPORT_READY : this->IsSelectedServiceConnected() ? STR_GEO_CONNECTED : STR_GEO_EXPORT_REFUSED);
			const auto actual = GetGeothermalControlInspection(this->inspected);
			if (actual.vent != nullptr) text += "\n" + GetString(STR_GEO_SURVEY_CAPACITY, actual.vent->thermal_capacity_w);
			const auto quote = this->QuoteFunding();
			if (quote.Succeeded()) text += "\n" + GetString(STR_GEO_FUND_QUOTE, quote.GetCost());
			return text;
		}
		default: return std::nullopt;
	}
}
