/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_gui.cpp Native station foldout requests; paid history, packets and commands retain all simulation authority.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Groups14B-14C; canonical Pending-only native refunds.
 */
#include "stdafx.h"
#include "surface_supply_gui.h"
#include "surface_supply_cmd.h"
#include "surface_supply_order_runtime.h"
#include "economy_base.h"
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

/** Encode only the existing synchronized command's canonical label tuple.
 * @param label Four original canonical bytes, independent of runtime cargo slots.
 * @return Big-endian wire value consumed by CmdPlaceSurfaceSupplyOrder.
 */
static uint32_t SurfaceSupplyLabelValue(CargoLabel label)
{
	return (uint32_t{label[0]} << 24) | (uint32_t{label[1]} << 16) | (uint32_t{label[2]} << 8) | label[3];
}

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
	for (const auto &row : CaptureSurfaceSupplyOrders().records) {
		// Only this player's actual source history keeps the panel visible;
		// another station's record cannot reopen ordering or cancellation here.
		if (row.origin == this->station && row.owner == _local_company) return true;
	}
	return false;
}

/** @copydoc SurfaceSupplyControls::SelectedOrder */
const SurfaceSupplyOrderRecord *SurfaceSupplyControls::SelectedOrder() const
{
	const auto *row = GetSurfaceSupplyOrder(this->order);
	// A dropdown is only an identity request; takeover or a stale callback
	// cannot expose a different station's cancellation entitlement.
	return row != nullptr && row->origin == this->station && row->owner == _local_company ? row : nullptr;
}

/** @copydoc SurfaceSupplyControls::QuotePurchase */
CommandCost SurfaceSupplyControls::QuotePurchase() const
{
	AutoRestoreBackup actor{_current_company, _local_company};
	return CmdPlaceSurfaceSupplyOrder({}, this->station, SurfaceSupplyLabelValue(this->label), this->quantity, GetNextSurfaceSupplyOrderID());
}

/** @copydoc SurfaceSupplyControls::QuoteCancellation */
CommandCost SurfaceSupplyControls::QuoteCancellation() const
{
	AutoRestoreBackup actor{_current_company, _local_company};
	const auto *row = this->SelectedOrder();
	if (row == nullptr) return CMD_ERROR;
	if (row->Legacy() != nullptr) {
		const auto quote = this->QuoteLegacyCancellation();
		if (!quote) return CMD_ERROR;
		return CmdCancelLegacySurfaceSupplyOrder({}, this->order, quote->quantity);
	}
	return CmdCancelSurfaceSupplyOrder({}, this->order);
}

/** @copydoc SurfaceSupplyControls::QuoteLegacyCancellation */
std::optional<LegacySurfaceSupplyCancellation> SurfaceSupplyControls::QuoteLegacyCancellation() const
{
	AutoRestoreBackup actor{_current_company, _local_company};
	if (this->SelectedOrder() == nullptr) return std::nullopt;
	return PrepareLegacySurfaceSupplyCancellation(this->order);
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
	// Ownership/profile changes can arrive after construction. Only local
	// layout follows them here; a hidden stale panel never grants a command.
	if (window.GetWidget<NWidgetStacked>(WID_SV_SUPPLY_OUTER)->SetDisplayedPlane(available ? 0 : SZSP_NONE)) window.ReInit();
	window.SetWidgetDisabledState(WID_SV_SUPPLIES, !available);
	window.SetWidgetLoweredState(WID_SV_SUPPLIES, this->expanded);
	window.SetWidgetsDisabledState(!available, WID_SV_SUPPLY_CARGO, WID_SV_SUPPLY_QUANTITY, WID_SV_SUPPLY_ORDERS);
	// An unfinished/invalid quantity request cannot buy the previous valid
	// quantity by accident; current command admission remains the final authority.
	window.SetWidgetDisabledState(WID_SV_SUPPLY_BUY, !available || this->quantity_query || !this->guidance.empty() || this->QuotePurchase().Failed());
	// Native refunds require Pending; legacy refunds require their separately
	// admitted original source stock. Neither path acquires the other's rights.
	window.SetWidgetDisabledState(WID_SV_SUPPLY_CANCEL, !available || this->SelectedOrder() == nullptr || this->QuoteCancellation().Failed());
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
				const auto *terms = GetSurfaceSupplyDefinition(label);
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
			// A delayed click cannot bypass a pending/invalid quantity editor
			// merely because the parent has not painted its disabled state yet.
			if (this->quantity_query || !this->guidance.empty()) break;
			const auto quote = this->QuotePurchase();
			if (quote.Failed()) break;
			const auto intent = GetNextSurfaceSupplyOrderID();
			if (Command<Commands::PlaceSurfaceSupplyOrder>::Post(STR_ERROR_SURFACE_SUPPLY_ACTION, this->station, SurfaceSupplyLabelValue(this->label), this->quantity, intent)) this->order = intent;
			break;
		}
		case WID_SV_SUPPLY_ORDERS: {
			DropDownList list; this->offered_ids.clear(); int selected = -1;
			// List real retained source history, including closed rows so refunds
			// and dispatch cannot disappear from the player's account after completion.
			for (const auto &row : CaptureSurfaceSupplyOrders().records) {
				// Only this player's actual source history belongs in this request
				// list; neither an ordinal nor a foreign receipt grants authority.
				if (row.origin != this->station || row.owner != _local_company) continue;
				const auto option = static_cast<int>(this->offered_ids.size());
				this->offered_ids.push_back(row.id);
				if (row.id == this->order) selected = option;
				list.push_back(MakeDropDownListStringItem(GetString(STR_SURFACE_SUPPLY_ORDER_CHOICE, row.id, SurfaceSupplyCargoName(row.label), row.quantity), option));
			}
			if (!list.empty()) ShowDropDownList(&window, std::move(list), selected, widget);
			break;
		}
		case WID_SV_SUPPLY_CANCEL: {
			const auto *row = this->SelectedOrder();
			if (row == nullptr) break;
			if (row->Legacy() != nullptr) {
				const auto quote = this->QuoteLegacyCancellation();
				// A delayed click may request only the exact quantity/refund last
				// rendered. If real stock or ownership changed, redraw the new
				// facts instead of silently closing a different remaining amount.
				if (!quote || !this->displayed_legacy_cancellation || *quote != *this->displayed_legacy_cancellation) {
					this->displayed_legacy_cancellation.reset(); break;
				}
				Command<Commands::CancelLegacySurfaceSupplyOrder>::Post(STR_ERROR_SURFACE_SUPPLY_ACTION, this->order, quote->quantity);
			} else {
				if (this->QuoteCancellation().Failed()) break;
				Command<Commands::CancelSurfaceSupplyOrder>::Post(STR_ERROR_SURFACE_SUPPLY_ACTION, this->order);
			}
			this->displayed_legacy_cancellation.reset();
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
	// Parent controls keep their own callbacks. A delayed supply menu cannot
	// mutate draft selection after takeover, profile change or source removal.
	if (widget != WID_SV_SUPPLY_CARGO && widget != WID_SV_SUPPLY_ORDERS) return false;
	if (!this->IsVisible()) return true;
	if (widget == WID_SV_SUPPLY_CARGO) {
		// Delayed/malformed dropdown callbacks cannot select a cargo outside
		// the four native service options or index past the menu's fixed domain.
		if (index >= 0 && static_cast<size_t>(index) < _surface_supply_choices.size()) { this->label = _surface_supply_choices[index]; this->guidance.clear(); }
	} else if (widget == WID_SV_SUPPLY_ORDERS) {
		this->displayed_legacy_cancellation.reset();
		// Only an option from this displayed menu may resolve an exact ID.
		// An arbitrary/narrowed number never becomes a receipt lookup request.
		if (index >= 0 && static_cast<size_t>(index) < this->offered_ids.size()) {
			const auto *row = GetSurfaceSupplyOrder(this->offered_ids[index]);
			// Only an actual same-source/same-player identity becomes selected;
			// the later cancellation command independently rechecks stock and ownership.
			if (row != nullptr && row->origin == this->station && row->owner == _local_company) this->order = row->id;
		}
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
			const auto *terms = GetSurfaceSupplyDefinition(this->label);
			const auto tariff = CalculateSurfaceSupplyCost(this->label, 1, _economy.inflation_prices);
			// Missing canonical/financial terms cannot advertise a free quote;
			// actual command admission still determines whether this request may execute.
			if (terms == nullptr || !tariff) return GetString(STR_ERROR_SURFACE_SUPPLY_REFUSED);
			return GetString(STR_SURFACE_SUPPLY_TERMS, terms->introduction_year.base(), *tariff);
		}
		case WID_SV_SUPPLY_LIMIT: {
			const auto *terms = GetSurfaceSupplyDefinition(this->label);
			if (terms == nullptr) return GetString(STR_ERROR_SURFACE_SUPPLY_REFUSED);
			const auto used = GetSurfaceSupplyOrderDayUsage(_local_company, this->label);
			if (!used) return GetString(STR_ERROR_SURFACE_SUPPLY_REFUSED);
			const uint32_t remaining = *used >= terms->capacity_per_day ? 0 : terms->capacity_per_day - *used;
			return GetString(STR_SURFACE_SUPPLY_LIMIT, remaining, terms->capacity_per_day);
		}
		case WID_SV_SUPPLY_QUOTE: {
			if (!this->guidance.empty()) return this->guidance;
			const auto quote = this->QuotePurchase();
			return quote.Failed() ? GetString(STR_ERROR_SURFACE_SUPPLY_REFUSED) : GetString(STR_SURFACE_SUPPLY_TOTAL, quote.GetCost());
		}
		case WID_SV_SUPPLY_ORDERS: {
			const auto *row = this->SelectedOrder();
			return row == nullptr ? GetString(STR_SURFACE_SUPPLY_SELECT_ORDER) : GetString(STR_SURFACE_SUPPLY_ORDER_NUMBER, row->id, SurfaceSupplyCargoName(row->label));
		}
		case WID_SV_SUPPLY_HISTORY: {
			const auto *row = this->SelectedOrder();
			if (row == nullptr) return GetString(STR_SURFACE_SUPPLY_NONPAYING);
			if (const auto *legacy = row->Legacy()) return GetString(STR_SURFACE_SUPPLY_HISTORY, row->quantity, legacy->dispatched, legacy->refunded,
				GetString(legacy->closed ? STR_SURFACE_SUPPLY_CLOSED : STR_SURFACE_SUPPLY_OPEN));
			if (const auto *native = row->Native()) {
				StringID status = STR_ERROR_SURFACE_SUPPLY_REFUSED;
				switch (native->state) {
					case SurfaceSupplyOrderState::Pending: status = STR_SURFACE_SUPPLY_PENDING; break;
					case SurfaceSupplyOrderState::Fulfilled: status = STR_SURFACE_SUPPLY_FULFILLED; break;
					case SurfaceSupplyOrderState::Cancelled: status = STR_SURFACE_SUPPLY_CANCELLED; break;
					case SurfaceSupplyOrderState::Lost: status = STR_SURFACE_SUPPLY_LOST; break;
				}
				return GetString(STR_SURFACE_SUPPLY_NATIVE_HISTORY, row->quantity, GetString(status));
			}
			return GetString(STR_ERROR_SURFACE_SUPPLY_REFUSED);
		}
		case WID_SV_SUPPLY_CANCEL: {
			const auto *row = this->SelectedOrder();
			// Only rendering remembers a legacy request. The saved snapshot is
			// local click intent, never permission to skip command recapture.
			this->displayed_legacy_cancellation.reset();
			if (row != nullptr && row->Legacy() != nullptr) {
				this->displayed_legacy_cancellation = this->QuoteLegacyCancellation();
				if (this->displayed_legacy_cancellation) return GetString(STR_SURFACE_SUPPLY_LEGACY_REFUND,
					this->displayed_legacy_cancellation->quantity, this->displayed_legacy_cancellation->refund);
				return GetString(STR_SURFACE_SUPPLY_LEGACY_CANCEL);
			}
			const auto quote = this->QuoteCancellation();
			if (quote.Succeeded()) return GetString(STR_SURFACE_SUPPLY_REFUND, row->quantity, -quote.GetCost());
			return GetString(STR_SURFACE_SUPPLY_CANCEL);
		}
		default: return std::nullopt;
	}
}
