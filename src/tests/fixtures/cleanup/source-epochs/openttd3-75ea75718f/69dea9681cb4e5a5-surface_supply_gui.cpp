/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_gui.cpp Native station foldout requests; paid history, packets and commands retain all simulation authority.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 */
#include "stdafx.h"
#include "surface_supply_gui.h"
#include "surface_supply_cmd.h"
#include "multilayer_bootstrap.h"
#include "multilayer_content.h"
#include "map_layer.h"
#include "station_base.h"
#include "company_base.h"
#include "company_func.h"
#include "window_gui.h"
#include "window_func.h"
#include "strings_func.h"
#include "textbuf_gui.h"
#include "dropdown_func.h"
#include "dropdown_type.h"
#include "command_func.h"
#include "cargotype.h"
#include "timer/timer_game_economy.h"
#include "widgets/station_widget.h"
#include "table/strings.h"
#include <charconv>
#include "safeguards.h"

static constexpr std::array<CargoLabel, 4> _surface_supply_choices{CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}}; ///< UI choices only; canonical source terms/admission remain authoritative.

/** @param label Existing stable meaning. @return Native translated cargo name, with label text only for an absent registry entry. */
static std::string SurfaceSupplyCargoName(CargoLabel label)
{
	const CargoType cargo = GetCargoTypeByLabel(label);
	return cargo == INVALID_CARGO ? label.AsString() : GetString(CargoSpec::Get(cargo)->name);
}

/** @copydoc SurfaceSupplyControls::SurfaceSupplyControls */
SurfaceSupplyControls::SurfaceSupplyControls(StationID station) : station(station) {}

/** @copydoc SurfaceSupplyControls::IsVisible */
bool SurfaceSupplyControls::IsVisible() const
{
	const Station *source = Station::GetIfValid(this->station);
	// Only the actual player's native Surface service belongs in this foldout;
	// another profile, a hidden layer or a foreign station grants no ordering entry.
	if (source == nullptr || !Company::IsValidID(_local_company) || source->owner != _local_company ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry || !MultiLayerMap::IsTileInLayer(source->xy, MapLayer::Surface)) return false;
	if (source->facilities.Any({StationFacility::Train, StationFacility::TruckStop})) return true;
	// Removing the final freight berth does not erase refundable source stock.
	// Retained native orders keep cancellation reachable until the station itself disappears.
	for (const auto &row : GetSurfaceSupplyOrderHistory()) {
		if (row.station == this->station && row.owner == _local_company) return true;
	}
	return false;
}

/** @copydoc SurfaceSupplyControls::SelectedOrder */
const SurfaceSupplyOrder *SurfaceSupplyControls::SelectedOrder() const
{
	const auto *row = GetSurfaceSupplyOrder(this->order);
	// A dropdown is only an identity request; takeover or a stale callback
	// cannot expose a different station's cancellation entitlement.
	return row != nullptr && row->station == this->station && row->owner == _local_company ? row : nullptr;
}

/** @copydoc SurfaceSupplyControls::QuotePurchase */
SurfaceSupplyQuote SurfaceSupplyControls::QuotePurchase() const
{
	AutoRestoreBackup actor{_current_company, _local_company};
	return PrepareSurfaceSupplyOrder(this->station, this->label, this->quantity);
}

/** @copydoc SurfaceSupplyControls::QuoteCancellation */
SurfaceSupplyCancellation SurfaceSupplyControls::QuoteCancellation() const
{
	AutoRestoreBackup actor{_current_company, _local_company};
	return PrepareSurfaceSupplyCancellation(this->order);
}

/** @copydoc SurfaceSupplyControls::Configure */
void SurfaceSupplyControls::Configure(Window &window) const
{
	window.GetWidget<NWidgetStacked>(WID_SV_SUPPLY_OUTER)->SetDisplayedPlane(this->IsVisible() ? 0 : SZSP_NONE);
	window.GetWidget<NWidgetStacked>(WID_SV_SUPPLY_STACK)->SetDisplayedPlane(this->expanded ? 0 : SZSP_NONE);
}

/** @copydoc SurfaceSupplyControls::Refresh */
void SurfaceSupplyControls::Refresh(Window &window) const
{
	const bool available = this->IsVisible();
	window.SetWidgetDisabledState(WID_SV_SUPPLIES, !available);
	window.SetWidgetLoweredState(WID_SV_SUPPLIES, this->expanded);
	window.SetWidgetsDisabledState(!available, WID_SV_SUPPLY_CARGO, WID_SV_SUPPLY_QUANTITY, WID_SV_SUPPLY_ORDERS);
	// An unfinished/invalid quantity request cannot buy the previous valid
	// quantity by accident; current command admission remains the final authority.
	window.SetWidgetDisabledState(WID_SV_SUPPLY_BUY, !available || this->quantity_query || !this->guidance.empty() || this->QuotePurchase().error != SurfaceSupplyError::None);
	// Refunds require the selected actual same-source order and its exact native
	// remaining stock. Closed, shipped-only, foreign or stale claims never gain credit here.
	window.SetWidgetDisabledState(WID_SV_SUPPLY_CANCEL, !available || this->SelectedOrder() == nullptr || this->QuoteCancellation().error != SurfaceSupplyError::None);
}

/** @copydoc SurfaceSupplyControls::OnClick */
bool SurfaceSupplyControls::OnClick(Window &window, WidgetID widget)
{
	// Only this contiguous station foldout belongs to the component; unrelated
	// cargo sorting, station renaming and launch controls keep their native handlers.
	if (widget < WID_SV_SUPPLIES || widget > WID_SV_SUPPLY_HISTORY) return false;
	// Recheck native ownership after delayed input, even before another paint;
	// a stale enabled button grants no service or query authority at a foreign source.
	if (window.IsWidgetDisabled(widget) || !this->IsVisible()) return true;
	switch (widget) {
		case WID_SV_SUPPLIES:
			this->expanded = !this->expanded;
			window.GetWidget<NWidgetStacked>(WID_SV_SUPPLY_STACK)->SetDisplayedPlane(this->expanded ? 0 : SZSP_NONE);
			window.ReInit();
			break;
		case WID_SV_SUPPLY_CARGO: {
			DropDownList list;
			// These four choices borrow the source's original seed terms; menu
			// selection neither installs a cargo nor reserves its daily allowance.
			for (size_t index = 0; index < _surface_supply_choices.size(); ++index) {
				const auto label = _surface_supply_choices[index];
				const auto *terms = GetSurfaceSupplyTerms(label);
				if (terms == nullptr) continue;
				list.push_back(MakeDropDownListStringItem(GetString(STR_SURFACE_SUPPLY_CHOICE, SurfaceSupplyCargoName(label), terms->introduction_year.base()), static_cast<int>(index)));
			}
			ShowDropDownList(&window, std::move(list), static_cast<int>(std::ranges::find(_surface_supply_choices, this->label) - _surface_supply_choices.begin()), widget);
			break;
		}
		case WID_SV_SUPPLY_QUANTITY:
			CloseWindowByClass(WindowClass::QueryString); // Finish any old parent's callback before marking this new editor as ours.
			this->guidance.clear(); this->quantity_query = true;
			ShowQueryString(fmt::format("{}", this->quantity), STR_SURFACE_SUPPLY_QUANTITY_QUERY, 6, &window, CS_NUMERAL, {QueryStringFlag::AcceptUnchanged});
			break;
		case WID_SV_SUPPLY_BUY: {
			const auto quote = this->QuotePurchase();
			if (quote.error != SurfaceSupplyError::None) break;
			if (Command<Commands::OrderSurfaceSupply>::Post(STR_ERROR_SURFACE_SUPPLY_ACTION, this->station, this->label, this->quantity, quote.next_order)) this->order = quote.next_order;
			break;
		}
		case WID_SV_SUPPLY_ORDERS: {
			DropDownList list;
			// List real retained source history, including closed rows so refunds
			// and dispatch cannot disappear from the player's account after completion.
			for (const auto &row : GetSurfaceSupplyOrderHistory()) {
				if (row.station != this->station || row.owner != _local_company) continue;
				list.push_back(MakeDropDownListStringItem(GetString(STR_SURFACE_SUPPLY_ORDER_CHOICE, row.id.base(), SurfaceSupplyCargoName(row.label), row.quantity), static_cast<int>(row.id.base())));
			}
			if (!list.empty()) ShowDropDownList(&window, std::move(list), static_cast<int>(this->order.base()), widget);
			break;
		}
		case WID_SV_SUPPLY_CANCEL: {
			if (this->SelectedOrder() == nullptr) break;
			const auto quote = this->QuoteCancellation();
			if (quote.error != SurfaceSupplyError::None) break;
			Command<Commands::CancelSurfaceSupply>::Post(STR_ERROR_SURFACE_SUPPLY_ACTION, this->order, quote.quantity);
			break;
		}
		default: break;
	}
	window.SetDirty();
	return true;
}

/** @copydoc SurfaceSupplyControls::OnDropdownSelect */
bool SurfaceSupplyControls::OnDropdownSelect(Window &window, WidgetID widget, int index)
{
	if (widget == WID_SV_SUPPLY_CARGO) {
		// Delayed/malformed dropdown callbacks cannot select a cargo outside
		// the four native service options or index past the menu's fixed domain.
		if (index >= 0 && static_cast<size_t>(index) < _surface_supply_choices.size()) { this->label = _surface_supply_choices[index]; this->guidance.clear(); }
	} else if (widget == WID_SV_SUPPLY_ORDERS) {
		if (index > 0) {
			const auto *row = GetSurfaceSupplyOrder(SurfaceSupplyOrderID{static_cast<uint32_t>(index)});
			// Only an actual same-source/same-player identity becomes selected;
			// the later cancellation command independently rechecks stock and ownership.
			if (row != nullptr && row->station == this->station && row->owner == _local_company) this->order = row->id;
		}
	} else {
		return false;
	}
	window.SetDirty();
	return true;
}

/** @copydoc SurfaceSupplyControls::OnQueryTextFinished */
bool SurfaceSupplyControls::OnQueryTextFinished(Window &window, const std::optional<std::string> &text)
{
	if (!this->quantity_query) return false;
	this->quantity_query = false;
	if (text) {
		uint16_t units = 0;
		const auto parsed = std::from_chars(text->data(), text->data() + text->size(), units);
		// Accept the complete positive native quantity only; malformed suffixes,
		// overflow and zero must never silently buy a previous or truncated amount.
		if (parsed.ec != std::errc{} || parsed.ptr != text->data() + text->size() || units == 0) {
			this->guidance = GetString(STR_SURFACE_SUPPLY_BAD_QUANTITY);
		} else {
			this->quantity = units; this->guidance.clear();
		}
	}
	window.SetDirty();
	return true;
}

/** @copydoc SurfaceSupplyControls::GetWidgetText */
std::optional<std::string> SurfaceSupplyControls::GetWidgetText(WidgetID widget) const
{
	AutoRestoreBackup actor{_current_company, _local_company};
	switch (widget) {
		case WID_SV_SUPPLY_CARGO: return SurfaceSupplyCargoName(this->label);
		case WID_SV_SUPPLY_QUANTITY: return GetString(STR_SURFACE_SUPPLY_QUANTITY, this->quantity);
		case WID_SV_SUPPLY_TERMS: {
			const auto *terms = GetSurfaceSupplyTerms(this->label);
			const auto tariff = GetSurfaceSupplyUnitPrice(this->label);
			// Missing canonical/financial terms cannot advertise a free quote;
			// actual command admission still determines whether this request may execute.
			if (terms == nullptr || !tariff) return GetString(STR_ERROR_SURFACE_SUPPLY_REFUSED);
			return GetString(STR_SURFACE_SUPPLY_TERMS, terms->introduction_year.base(), *tariff);
		}
		case WID_SV_SUPPLY_LIMIT: {
			const auto *terms = GetSurfaceSupplyTerms(this->label);
			if (terms == nullptr) return GetString(STR_ERROR_SURFACE_SUPPLY_REFUSED);
			uint32_t used = 0;
			// Read actual retained history even before a technology date or after
			// a refund; neither circumstance renews the company-wide economy-day cap.
			for (const auto &row : GetSurfaceSupplyOrderHistory()) {
				if (row.owner == _local_company && row.label == this->label && row.economy_day == TimerGameEconomy::date.base()) used += row.quantity;
			}
			const uint32_t remaining = used >= terms->capacity_per_day ? 0 : terms->capacity_per_day - used;
			return GetString(STR_SURFACE_SUPPLY_LIMIT, remaining, terms->capacity_per_day);
		}
		case WID_SV_SUPPLY_QUOTE: {
			if (!this->guidance.empty()) return this->guidance;
			const auto quote = this->QuotePurchase();
			const auto command = CmdOrderSurfaceSupply({}, this->station, this->label, this->quantity, quote.next_order);
			return command.Failed() ? GetString(command.GetErrorMessage()) : GetString(STR_SURFACE_SUPPLY_TOTAL, quote.cost);
		}
		case WID_SV_SUPPLY_ORDERS: {
			const auto *row = this->SelectedOrder();
			return row == nullptr ? GetString(STR_SURFACE_SUPPLY_SELECT_ORDER) : GetString(STR_SURFACE_SUPPLY_ORDER_NUMBER, row->id.base(), SurfaceSupplyCargoName(row->label));
		}
		case WID_SV_SUPPLY_HISTORY: {
			const auto *row = this->SelectedOrder();
			if (row == nullptr) return GetString(STR_SURFACE_SUPPLY_NONPAYING);
			return GetString(STR_SURFACE_SUPPLY_HISTORY, row->quantity, row->dispatched, row->refunded,
				GetString(row->closed ? STR_SURFACE_SUPPLY_CLOSED : STR_SURFACE_SUPPLY_OPEN));
		}
		case WID_SV_SUPPLY_CANCEL: {
			const auto quote = this->QuoteCancellation();
			return quote.error == SurfaceSupplyError::None ? GetString(STR_SURFACE_SUPPLY_REFUND, quote.quantity, quote.refund) : GetString(STR_SURFACE_SUPPLY_CANCEL);
		}
		default: return std::nullopt;
	}
}
