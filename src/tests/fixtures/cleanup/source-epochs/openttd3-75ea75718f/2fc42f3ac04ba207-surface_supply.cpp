/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply.cpp One company-wide paid seed order owner over existing station cargo packets.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups13CB-13CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#include "stdafx.h"
#include "surface_supply.h"
#include "multilayer_bootstrap.h"
#include "multilayer_content.h"
#include "company_base.h"
#include "company_func.h"
#include "command_type.h"
#include "station_base.h"
#include "station_supply.h"
#include "cargo_custody.h"
#include <set>
#include "map_layer.h"
#include "timer/timer_game_calendar.h"
#include "timer/timer_game_economy.h"
#include "window_func.h"
#include "safeguards.h"

static std::vector<SurfaceSupplyOrder> _surface_supply_orders; ///< Purchase/cap/refund history only; no physical cargo quantity lives here.
static uint32_t _next_surface_supply_order = 1; ///< Monotonic cursor; packet value zero always means ordinary freight.
static std::optional<SurfaceSupplyState> _pending_surface_supply; ///< Unpublished complete load candidate; native packets stay in their reconstructed physical owners.

/** @copydoc GetSurfaceSupplyOrder */
const SurfaceSupplyOrder *GetSurfaceSupplyOrder(SurfaceSupplyOrderID id)
{
	// Complete retained history starts at one and never drops/reuses a row.
	// Resolve its native identity directly without scanning every past purchase at each unload.
	if (id.base() == 0 || id.base() > _surface_supply_orders.size()) return nullptr;
	const auto &order = _surface_supply_orders[id.base() - 1];
	return order.id == id ? &order : nullptr;
}

/** @copydoc GetNextSurfaceSupplyOrderID */
SurfaceSupplyOrderID GetNextSurfaceSupplyOrderID() { return SurfaceSupplyOrderID{_next_surface_supply_order}; }

/** @copydoc ResetSurfaceSupplyOrders */
void ResetSurfaceSupplyOrders()
{
	_surface_supply_orders.clear();
	_next_surface_supply_order = 1;
	_pending_surface_supply.reset();
	ResetSurfaceSupplyLoadStaging();
}

/**
 * Select only this dispatch's four canonical construction seeds, without inventing cargo or price records.
 * @param label Existing profile meaning requested by the caller.
 * @return Canonical immutable terms, or nullptr for an unselected/unknown seed.
 */
const MultiLayerSurfaceSupply *GetSurfaceSupplyTerms(CargoLabel label)
{
	// First geothermal construction uses these four original meanings. Other
	// canonical seeds belong to their later procurement/launch consumers.
	if (label != CargoLabel{"STEL"} && label != CargoLabel{"RFMT"} && label != CargoLabel{"ELEC"} && label != CargoLabel{"BATY"}) return nullptr;
	const auto terms = GetMultiLayerSurfaceSupplies();
	const auto found = std::ranges::find(terms, label, &MultiLayerSurfaceSupply::label);
	return found == terms.end() ? nullptr : &*found;
}

/** @copydoc GetSurfaceSupplyUnitPrice */
std::optional<Money> GetSurfaceSupplyUnitPrice(CargoLabel label)
{
	const auto *terms = GetSurfaceSupplyTerms(label);
	// Invalid/missing tariffs must never display or settle a free import;
	// the same bounded financial reference serves both presentation and actual admission.
	if (terms == nullptr || _economy.inflation_prices == 0 || _economy.inflation_prices > UINT64_MAX / terms->price_per_unit) return std::nullopt;
	const uint64_t price = (uint64_t{terms->price_per_unit} * _economy.inflation_prices) >> 16;
	if (price == 0 || price > uint64_t{INT64_MAX}) return std::nullopt;
	return Money{static_cast<int64_t>(price)};
}

/** @copydoc GetSurfaceSupplyOrderHistory */
std::span<const SurfaceSupplyOrder> GetSurfaceSupplyOrderHistory() { return _surface_supply_orders; }

/** @copydoc PrepareSurfaceSupplyOrder */
SurfaceSupplyQuote PrepareSurfaceSupplyOrder(StationID station, CargoLabel label, uint16_t quantity)
{
	SurfaceSupplyQuote quote;
	quote.owner = _current_company; quote.station = station; quote.label = label; quote.quantity = quantity;
	quote.next_order = GetNextSurfaceSupplyOrderID(); quote.economy_day = TimerGameEconomy::date.base();
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) { quote.error = SurfaceSupplyError::Profile; return quote; }
	if (!Company::IsValidID(quote.owner)) { quote.error = SurfaceSupplyError::Company; return quote; }
	const Station *pickup = Station::GetIfValid(station);
	// The station itself must be a real owned Surface freight facility. A bus
	// stop, ghost, hidden-layer station or another company's same-XY sign cannot seed cargo.
	if (pickup == nullptr || pickup->owner != quote.owner || !MultiLayerMap::IsTileInLayer(pickup->xy, MapLayer::Surface) ||
		!pickup->facilities.Any({StationFacility::Train, StationFacility::TruckStop})) {
		quote.error = SurfaceSupplyError::Station; return quote;
	}
	const auto *terms = GetSurfaceSupplyTerms(label);
	quote.cargo = GetCargoTypeByLabel(label);
	// Both immutable seed terms and the active registered physical meaning
	// must exist; a translated name or unused runtime cargo slot grants no supply.
	if (terms == nullptr || quote.cargo == INVALID_CARGO) { quote.error = SurfaceSupplyError::Supply; return quote; }
	// Technology follows the saved calendar date; the economy ordering day
	// and cached year fields cannot grant or withhold imported construction supplies.
	if (TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date).year < terms->introduction_year) { quote.error = SurfaceSupplyError::Date; return quote; }
	if (quantity == 0) { quote.error = SurfaceSupplyError::Quantity; return quote; }
	// All this company's stations share one day's paid history. Shipment,
	// consumption, refunds and station copies cannot erase a used purchase allowance.
	for (const auto &order : _surface_supply_orders) {
		if (order.owner == quote.owner && order.economy_day == quote.economy_day && order.label == label) quote.day_used += order.quantity;
	}
	if (uint64_t{quote.day_used} + quantity > terms->capacity_per_day) { quote.error = SurfaceSupplyError::DailyCap; return quote; }
	quote.waiting = pickup->goods[quote.cargo].TotalCount();
	if (uint64_t{quote.waiting} + quantity > EXACT_STATION_SUPPLY_CAPACITY) { quote.error = SurfaceSupplyError::Stock; return quote; }
	// Tariffs are internal Money before ordinary price inflation. Refuse an
	// invalid/overflowing multiplier before computing or publishing a free/partial order.
	const auto unit_price = GetSurfaceSupplyUnitPrice(label);
	// The whole quantity must fit the native Money debit; a reference tariff
	// alone grants no free, truncated or overflowing purchase.
	if (!unit_price || *unit_price > Money{INT64_MAX / quantity}) { quote.error = SurfaceSupplyError::Funds; return quote; }
	quote.unit_price = *unit_price;
	quote.cost = *unit_price * quantity;
	if (quote.cost > GetAvailableMoneyForCommand()) { quote.error = SurfaceSupplyError::Funds; return quote; }
	if (!CargoPacket::CanAllocateItem()) { quote.error = SurfaceSupplyError::Pool; return quote; }
	// Retaining bounded paid history and never reusing a packet purpose ID
	// takes priority over accepting a purchase that cannot be represented safely.
	if (_surface_supply_orders.size() >= MAX_SURFACE_SUPPLY_ORDERS || _next_surface_supply_order == UINT32_MAX) {
		quote.error = SurfaceSupplyError::Limit; return quote;
	}
	return quote;
}

/** @copydoc PlaceSurfaceSupplyOrder */
SurfaceSupplyError PlaceSurfaceSupplyOrder(const SurfaceSupplyQuote &quote, bool execute)
{
	if (quote.error != SurfaceSupplyError::None) return quote.error;
	const auto current = PrepareSurfaceSupplyOrder(quote.station, quote.label, quote.quantity);
	if (current.error != SurfaceSupplyError::None) return current.error;
	if (current != quote) return SurfaceSupplyError::Stale;
	if (!execute) return SurfaceSupplyError::None;
	// Acquire history storage before allocating material or charging its owner.
	// Geometric bounded growth avoids copying the whole retained purchase history for every order.
	if (_surface_supply_orders.size() == _surface_supply_orders.capacity()) {
		_surface_supply_orders.reserve(std::min(MAX_SURFACE_SUPPLY_ORDERS, std::max(size_t{16}, _surface_supply_orders.capacity() * 2)));
	}
	CargoPacket *packet = CargoPacket::Create(quote.station, quote.quantity, Source{Source::Invalid, SourceType::Industry});
	packet->SetCompanySupplyOrder(quote.next_order.base());
	SurfaceSupplyOrder order;
	order.id = quote.next_order; order.owner = quote.owner; order.station = quote.station; order.label = quote.label;
	order.economy_day = quote.economy_day; order.quantity = quote.quantity; order.unit_price = quote.unit_price;
	order.calendar_date = TimerGameCalendar::date.base();
	_surface_supply_orders.push_back(order);
	++_next_surface_supply_order;
	Station *pickup = Station::Get(quote.station);
	pickup->goods[quote.cargo].GetOrCreateData().cargo.Append(packet, pickup->goods[quote.cargo].GetVia(quote.station)); // Preserve the existing native CargoDist next-hop choice for newly arrived stock.
	NotifyStationCargoSupply(quote.station, quote.cargo, quote.quantity); // Borrow ordinary graph/rating/events without fabricating an industry producer or replacing paid packet purpose.
	SubtractMoneyFromCompany(quote.owner, CommandCost(ExpensesType::Other, quote.cost));
	return SurfaceSupplyError::None;
}

/** @copydoc CaptureSurfaceSupplyState */
SurfaceSupplyState CaptureSurfaceSupplyState() { return {_next_surface_supply_order, _surface_supply_orders}; }

/** @copydoc ValidateSurfaceSupplyState */
SurfaceSupplyError ValidateSurfaceSupplyState(const SurfaceSupplyState &state, bool actual_references)
{
	// Complete bounded history forbids gaps, missing paid rows and a reused
	// identity cursor before any native packet/reference lookup or allocation.
	if (state.orders.size() > MAX_SURFACE_SUPPLY_ORDERS || state.next_order != state.orders.size() + 1) return SurfaceSupplyError::Corrupt;
	// Validate every retained purchase before any candidate gains live refund
	// or cap rights; shipment/consumption never excuses a malformed historical row.
	for (size_t index = 0; index < state.orders.size(); ++index) {
		const auto &order = state.orders[index];
		const auto *terms = GetSurfaceSupplyTerms(order.label);
		// Takeover can legitimately combine two previously paid day caps. Keep
		// that history; subsequent admission refuses more stock instead of rejecting or erasing old purchases.
		// Only real admitted whole purchases can remain in paid history. A
		// refunded amount has already retired cancellation, never reusable cargo rights.
		if (order.id.base() != index + 1 || terms == nullptr || order.quantity == 0 || order.quantity > terms->capacity_per_day ||
			order.refunded > order.quantity || order.dispatched > order.quantity - order.refunded || (order.refunded != 0 && !order.closed)) return SurfaceSupplyError::Corrupt;
		// Reserved/fake company ordinals and invalid historical dates cannot
		// manufacture an earlier purchase or a transferable daily allowance.
		if ((order.owner != CompanyID::Invalid() && order.owner.base() >= MAX_COMPANIES) || order.economy_day < 0 ||
			order.economy_day > EconomyTime::MAX_DATE.base() || order.calendar_date < 0 || order.calendar_date > CalendarTime::MAX_DATE.base()) return SurfaceSupplyError::Corrupt;
		if (order.calendar_date < TimerGameCalendar::ConvertYMDToDate(terms->introduction_year, 0, 1).base()) return SurfaceSupplyError::Corrupt;
		// The historical positive unit tariff is frozen, not recomputed using
		// today's inflation. Its complete refund product must fit actual Money.
		if (order.unit_price <= Money{0} || order.unit_price > Money{INT64_MAX / order.quantity}) return SurfaceSupplyError::Corrupt;
		if ((order.owner == CompanyID::Invalid() || order.station == StationID::Invalid()) && !order.closed) return SurfaceSupplyError::Corrupt;
		// Buyer loss also retires the source reference; an invalid company must
		// never retain a live station cancellation claim through identity reuse.
		if (order.owner == CompanyID::Invalid() && order.station != StationID::Invalid()) return SurfaceSupplyError::Corrupt;
		if (!actual_references) continue;
		if (order.owner != CompanyID::Invalid() && !Company::IsValidID(order.owner)) return SurfaceSupplyError::Corrupt;
		if (order.station != StationID::Invalid()) {
			const Station *station = Station::GetIfValid(order.station);
			// A retained source reference resolves to its actual owned Surface
			// queue; a removed/reused station cannot inherit old refund rights.
			if (station == nullptr || station->owner != order.owner || !MultiLayerMap::IsTileInLayer(station->xy, MapLayer::Surface)) return SurfaceSupplyError::Corrupt;
		}
	}
	if (!actual_references) return SurfaceSupplyError::None;
	if (!state.orders.empty() && GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return SurfaceSupplyError::Corrupt;
	std::set<CargoPacketID> seen;
	std::vector<uint32_t> outstanding(state.orders.size(), 0);
	std::vector<uint32_t> unshipped(state.orders.size(), 0);
	/**
	 * Check the candidate's purpose against each real physical owner, without touching that owner.
	 * @param packet Actual packet reference supplied by the shared native custody traversal.
	 * @param cargo Container's actual registered cargo meaning.
	 * @param station Actual goods-queue identity; invalid for vehicle, compartment and job owners.
	 * @return Whether purpose, label, uniqueness, refund custody and funded whole-unit bounds remain valid.
	 */
	const auto check_packet = [&](const CargoPacket &packet, CargoType cargo, StationID station) {
		const uint32_t id = packet.GetCompanySupplyOrder();
		if (id == 0) return !packet.HasCompanySupplyShipped();
		// A real purchase owns each funded fragment once. Unknown references,
		// another label or duplicate containers cannot be repaired into paying freight.
		if (id >= state.next_order || !seen.insert(packet.index).second || cargo >= NUM_CARGO || packet.Count() == 0) return false;
		const auto &order = state.orders[id - 1];
		if (CargoSpec::Get(cargo)->label != order.label) return false;
		// Only never-dispatched original source stock can remain refundable;
		// vehicle reservations, hub jobs and returned packets keep permanent dispatch history.
		// Native bankruptcy retains ownerless station signs and their queues
		// until normal deletion. Abandoned closed history grants no refund or cap
		// rights, while those real packets keep their permanent non-paying purpose.
		if (!packet.HasCompanySupplyShipped() && order.owner != CompanyID::Invalid() && (order.closed || station != order.station)) return false;
		outstanding[id - 1] += packet.Count();
		if (!packet.HasCompanySupplyShipped()) unshipped[id - 1] += packet.Count();
		// Historical first dispatch is independent of today's retained fragments.
		// Resetting a returned packet's flag cannot recover a shipped/consumed bill.
		return outstanding[id - 1] <= static_cast<uint32_t>(order.quantity - order.refunded) &&
			unshipped[id - 1] <= static_cast<uint32_t>(order.quantity - order.refunded - order.dispatched) &&
			outstanding[id - 1] - unshipped[id - 1] <= order.dispatched;
	};
	if (!VisitNativeCargoPackets(check_packet)) return SurfaceSupplyError::Corrupt;
	// Every physically allocated paid fragment needs one actual owner. An
	// orphan, or an ordinary packet with forged dispatch history, cannot survive loading.
	for (const CargoPacket *packet : CargoPacket::Iterate()) {
		if (packet->GetCompanySupplyOrder() == 0) {
			if (packet->HasCompanySupplyShipped()) return SurfaceSupplyError::Corrupt;
		} else if (!seen.contains(packet->index)) {
			return SurfaceSupplyError::Corrupt;
		}
	}
	return SurfaceSupplyError::None;
}

/** @copydoc StageLoadedSurfaceSupplyState */
void StageLoadedSurfaceSupplyState(SurfaceSupplyState state) { _pending_surface_supply = std::move(state); }

/** @copydoc AcceptLoadedSurfaceSupplyState */
bool AcceptLoadedSurfaceSupplyState()
{
	if (!_pending_surface_supply) return ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None;
	SurfaceSupplyState state = std::move(*_pending_surface_supply);
	_pending_surface_supply.reset();
	if (ValidateSurfaceSupplyState(state, true) != SurfaceSupplyError::None) return false;
	_next_surface_supply_order = state.next_order;
	_surface_supply_orders = std::move(state.orders);
	return true;
}

/**
 * Hash one explicit field in canonical big-endian byte order.
 * @param digest Existing FNV-1a diagnostic accumulator, never a mutation identity.
 * @param value Actual history/purpose scalar; pointers and object padding are excluded.
 */
static void HashSurfaceSupplyField(uint64_t &digest, uint64_t value)
{
	for (int shift = 56; shift >= 0; shift -= 8) {
		digest ^= static_cast<uint8_t>(value >> shift);
		digest *= 1099511628211ULL;
	}
}

/** @copydoc ComputeSurfaceSupplyDigest */
uint64_t ComputeSurfaceSupplyDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	HashSurfaceSupplyField(digest, _next_surface_supply_order);
	// Retained monotonic history has a stable native order; hashing never
	// creates a second paid/cap/physical stock authority.
	for (const auto &order : _surface_supply_orders) {
		for (uint64_t field : {uint64_t{order.id.base()}, uint64_t{order.owner.base()}, uint64_t{order.station.base()},
			uint64_t{static_cast<uint32_t>(order.economy_day)}, uint64_t{static_cast<uint32_t>(order.calendar_date)}, uint64_t{order.quantity},
			static_cast<uint64_t>(static_cast<int64_t>(order.unit_price)), uint64_t{order.refunded}, uint64_t{order.dispatched}, uint64_t{order.closed}}) HashSurfaceSupplyField(digest, field);
		for (uint8_t byte : order.label) HashSurfaceSupplyField(digest, byte);
	}
	// Native pool identity order makes purpose diagnostics stable across
	// ordinary reload; existing cargo/vehicle roots retain their physical ownership.
	for (const CargoPacket *packet : CargoPacket::Iterate()) {
		if (packet->GetCompanySupplyOrder() == 0 && !packet->HasCompanySupplyShipped()) continue; // Ordinary freight belongs to its existing diagnostic owners.
		HashSurfaceSupplyField(digest, packet->index.base()); HashSurfaceSupplyField(digest, packet->GetCompanySupplyOrder());
		HashSurfaceSupplyField(digest, packet->HasCompanySupplyShipped()); HashSurfaceSupplyField(digest, packet->Count());
	}
	return digest;
}

/** @copydoc PrepareSurfaceSupplyCancellation */
SurfaceSupplyCancellation PrepareSurfaceSupplyCancellation(SurfaceSupplyOrderID id)
{
	SurfaceSupplyCancellation quote;
	const auto *order = GetSurfaceSupplyOrder(id);
	// Missing/closed history has no fresh cancellation entitlement; retries
	// must not credit another invoice or search a replacement station identity.
	if (order == nullptr || order->closed) { quote.error = SurfaceSupplyError::Stale; return quote; }
	quote.order = *order;
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) { quote.error = SurfaceSupplyError::Profile; return quote; }
	// Only the actual surviving buyer may cancel. A takeover explicitly changes
	// that owner; a disappeared buyer grants no refund claim to another company.
	if (!Company::IsValidID(_current_company) || order->owner != _current_company) { quote.error = SurfaceSupplyError::Company; return quote; }
	const Station *source = Station::GetIfValid(order->station);
	// Partial facility removal may leave valid source stock without a rail/lorry
	// berth. It can still be cancelled, but a foreign/hidden/reused source cannot.
	if (source == nullptr || source->owner != order->owner || !MultiLayerMap::IsTileInLayer(source->xy, MapLayer::Surface)) { quote.error = SurfaceSupplyError::Station; return quote; }
	const CargoType cargo = GetCargoTypeByLabel(order->label);
	if (cargo == INVALID_CARGO) { quote.error = SurfaceSupplyError::Supply; return quote; }
	const uint64_t eligible = source->goods[cargo].HasData() ? source->goods[cargo].GetData().cargo.CountUnshippedCompanySupply(id.base()) : 0;
	// Actual remaining source units and a positive frozen whole tariff bound
	// the credit. No changed label/price/history can claim dispatched or consumed material.
	if (order->quantity == 0 || order->refunded > order->quantity || order->dispatched > order->quantity - order->refunded ||
		eligible > static_cast<uint16_t>(order->quantity - order->refunded - order->dispatched) ||
		order->unit_price <= Money{0} || order->unit_price > Money{INT64_MAX / order->quantity}) { quote.error = SurfaceSupplyError::Corrupt; return quote; }
	quote.quantity = static_cast<uint16_t>(eligible);
	quote.refund = order->unit_price * quote.quantity;
	const Money cash = Company::Get(order->owner)->money;
	// Only a positive wallet can overflow upward on a positive frozen refund;
	// subtracting a negative balance from INT64_MAX would itself overflow.
	if (cash > Money{0} && quote.refund > Money{INT64_MAX} - cash) { quote.error = SurfaceSupplyError::RefundLimit; return quote; }
	return quote;
}

/** @copydoc CancelSurfaceSupplyOrder */
SurfaceSupplyError CancelSurfaceSupplyOrder(const SurfaceSupplyCancellation &quote, bool execute)
{
	if (quote.error != SurfaceSupplyError::None) return quote.error;
	const auto current = PrepareSurfaceSupplyCancellation(quote.order.id);
	if (current.error != SurfaceSupplyError::None) return current.error;
	if (current != quote) return SurfaceSupplyError::Stale;
	if (!execute) return SurfaceSupplyError::None;
	Station *source = Station::Get(quote.order.station);
	const CargoType cargo = GetCargoTypeByLabel(quote.order.label);
	// The native queue rechecks its exact eligible count before the first
	// deletion. A stale partial source cannot leave credited usable material behind.
	if (quote.quantity != 0 && !source->goods[cargo].GetData().cargo.RemoveUnshippedCompanySupply(quote.order.id.base(), quote.quantity)) return SurfaceSupplyError::Stale;
	auto &order = _surface_supply_orders[quote.order.id.base() - 1];
	order.refunded += quote.quantity;
	order.closed = true;
	if (quote.refund != Money{0}) SubtractMoneyFromCompany(order.owner, CommandCost(ExpensesType::Other, -quote.refund));
	SetWindowDirty(WindowClass::StationView, source->index);
	InvalidateWindowData(WindowClass::StationList, order.owner);
	return SurfaceSupplyError::None;
}

/** @copydoc ForgetSurfaceSupplyStation */
void ForgetSurfaceSupplyStation(StationID station)
{
	// Source removal keeps native cargo-loss policy and paid day history. It
	// never touches carried/consumed units or turns deletion into an implicit refund.
	for (auto &order : _surface_supply_orders) {
		if (order.station != station) continue;
		order.closed = true;
		order.station = StationID::Invalid();
	}
}

/** @copydoc ChangeSurfaceSupplyOwnership */
void ChangeSurfaceSupplyOwnership(Owner old_owner, Owner new_owner)
{
	const bool takeover = Company::IsValidID(new_owner);
	// Every retained purchase follows the genuine company event. Combining
	// histories can exhaust today's cap; neither takeover nor loss resets spending.
	for (auto &order : _surface_supply_orders) {
		if (order.owner != old_owner) continue;
		if (takeover) {
			order.owner = new_owner;
		} else {
			order.owner = CompanyID::Invalid();
			order.station = StationID::Invalid();
			order.closed = true;
		}
	}
}

/** @copydoc RecordSurfaceSupplyDispatch */
bool RecordSurfaceSupplyDispatch(SurfaceSupplyOrderID id, uint16_t quantity)
{
	// Only an existing funded purchase can record a real first departure;
	// unknown/zero identities never grant another material or refund allowance.
	if (id.base() == 0 || id.base() > _surface_supply_orders.size() || quantity == 0) return false;
	auto &order = _surface_supply_orders[id.base() - 1];
	// Validate the whole historical remainder before incrementing. Loss/closure
	// does not erase a legitimate final native departure, but refunded or already
	// dispatched units can never be dispatched a second time.
	if (order.id != id || order.refunded > order.quantity || order.dispatched > order.quantity - order.refunded ||
		quantity > order.quantity - order.refunded - order.dispatched) return false;
	order.dispatched += quantity;
	return true;
}
