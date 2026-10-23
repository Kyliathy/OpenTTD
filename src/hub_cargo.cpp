/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file hub_cargo.cpp Dedicated station/job packet transfers; no fictitious vehicle or new cargo.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native cargo custody.
 */
#include "stdafx.h"
#include "hub_cargo.h"
#include "surface_supply_order_runtime.h"
#include <map>
#include "safeguards.h"

/** @copydoc HubCargoTransfer::CanMove(const StationCargoList &, uint) */
bool HubCargoTransfer::CanMove(const StationCargoList &source, uint quantity)
{
	// Whole positive source stock must exist before either packet allocation
	// or first-departure accounting can be admitted.
	if (quantity == 0 || quantity > source.AvailableCount()) return false;
	std::map<uint64_t, uint32_t> departures;
	// Preflight actual selected fragments in transfer order. Per-order totals
	// prevent two individually valid fragments from jointly overdrawing history.
	for (StationCargoList::ConstIterator it(source.packets.begin()); it != source.packets.end(); ++it) {
		const auto *packet = *it;
		const auto selected = static_cast<uint16_t>(std::min<uint>(quantity, packet->Count()));
		if (!packet->CanMarkCompanySupplyShipped(selected)) return false;
		// Returned and unlinked A stock have no new legacy event; only selected
		// never-shipped linked units consume the original finite remainder.
		if (packet->GetCompanySupplyOrder() != 0 && !packet->HasCompanySupplyShipped()) {
			auto &total = departures[packet->GetCompanySupplyOrder()]; total += selected;
			// Bound before narrowing, then ask the sole book about the aggregate;
			// this temporary admission total is not another cargo/history store.
			if (total > UINT16_MAX || !CanRecordSurfaceSupplyDispatch(packet->GetCompanySupplyOrder(), static_cast<uint16_t>(total))) return false;
		}
		if (quantity < packet->Count()) return CargoPacket::CanAllocateItem();
		quantity -= packet->Count();
		if (quantity == 0) return true;
	}
	return false;
}

/** @copydoc HubCargoTransfer::Move(StationCargoList &, StationCargoList &, uint) */
bool HubCargoTransfer::Move(StationCargoList &source, StationCargoList &target, uint quantity)
{
	// Moving within the same owner cannot be a new custody event, and a whole
	// admission refusal must precede every split, counter and cache mutation.
	if (&source == &target || !CanMove(source, quantity)) return false;
	// Commit the admitted native prefix in its original order. No callback or
	// external mutation intervenes between aggregate preflight and these events.
	while (quantity != 0) {
		StationCargoList::Iterator it(source.packets.begin());
		auto *packet = *it;
		const auto next = it.GetKey();
		const uint count = std::min<uint>(quantity, packet->Count());
		auto *moved = count == packet->Count() ? packet : packet->Split(count);
		assert(moved != nullptr); // The pure preflight guarantees the only required allocation.
		if (!moved->MarkCompanySupplyShipped()) {
			if (moved != packet) packet->Merge(moved);
			NOT_REACHED(); // Whole preflight covered every counter; never report a normal refusal after a partial committed prefix.
		}
		source.RemoveFromCache(moved, count);
		if (moved == packet) source.packets.erase(it);
		target.Append(moved, next);
		quantity -= count;
	}
	return true;
}

/** @copydoc HubCargoTransfer::Age */
void HubCargoTransfer::Age(StationCargoList &cargo)
{
	// Advance each actual fragment and its native weighted-age cache together;
	// saturated age stays saturated without changing physical quantity.
	for (StationCargoList::Iterator it(cargo.packets.begin()); it != cargo.packets.end(); ++it) {
		auto *packet = *it;
		if (packet->periods_in_transit == UINT16_MAX) continue;
		++packet->periods_in_transit;
		cargo.cargo_periods_in_transit += packet->Count();
	}
}

/** @copydoc HubCargoTransfer::Consume */
bool HubCargoTransfer::Consume(StationCargoList &source, uint quantity)
{
	// A partial or empty bill cannot consume inputs for an uncompleted recipe.
	if (quantity == 0 || quantity > source.AvailableCount()) return false;
	// Consume the admitted physical prefix through existing list/cache helpers;
	// history is not a replacement inventory and grants no automatic refund.
	while (quantity != 0) {
		StationCargoList::Iterator it(source.packets.begin());
		auto *packet = *it;
		const uint count = std::min<uint>(quantity, packet->Count());
		source.RemoveFromCache(packet, count);
		if (count == packet->Count()) { source.packets.erase(it); delete packet; }
		else packet->Reduce(count);
		quantity -= count;
	}
	return true;
}

/** @copydoc HubCargoTransfer::CanMove(const VehicleCargoList &, uint) */
bool HubCargoTransfer::CanMove(const VehicleCargoList &source, uint quantity)
{
	// Only a complete Keep-only native hold can participate; reserved or
	// delivery-designated units still belong to their original action owners.
	if (quantity == 0 || quantity > source.TotalCount() || source.ActionCount(VehicleCargoList::MoveToAction::Keep) != source.TotalCount()) return false;
	// Every selected linked carrier fragment must already have left its source;
	// this transfer cannot legitimize forged never-shipped in-transit stock.
	for (const auto *packet : source.packets) {
		if (packet->GetCompanySupplyOrder() != 0 && !packet->HasCompanySupplyShipped()) return false;
		if (!packet->CanMarkCompanySupplyShipped(static_cast<uint16_t>(std::min<uint>(quantity, packet->Count())))) return false;
		if (quantity < packet->Count()) return CargoPacket::CanAllocateItem();
		quantity -= packet->Count(); if (quantity == 0) return true;
	}
	return false;
}
/** @copydoc HubCargoTransfer::Move(StationCargoList &, VehicleCargoList &, uint, TileIndex) */
bool HubCargoTransfer::Move(StationCargoList &source, VehicleCargoList &target, uint quantity, TileIndex tile)
{
	// Existing destination action ranges and whole source admission must agree
	// before source rights or native carrier/cache ownership changes.
	if (target.ActionCount(VehicleCargoList::MoveToAction::Keep) != target.TotalCount() || !CanMove(source, quantity)) return false;
	// Move only the admitted native prefix; the actual split fragment records
	// first departure before distance/custody updates, never its unsplit parent.
	while (quantity != 0) {
		StationCargoList::Iterator it(source.packets.begin()); auto *packet = *it; const auto next = it.GetKey();
		const uint count = std::min<uint>(quantity, packet->Count());
		auto *moved = count == packet->Count() ? packet : packet->Split(count); assert(moved != nullptr);
		if (!moved->MarkCompanySupplyShipped()) {
			if (moved != packet) packet->Merge(moved);
			NOT_REACHED(); // Aggregate admission makes this transition no-fail in the single native simulation thread.
		}
		source.RemoveFromCache(moved, count); if (moved == packet) source.packets.erase(it);
		moved->SetNextHop(next); moved->UpdateLoadingTile(tile); target.Append(moved); quantity -= count;
	}
	return true;
}
/** @copydoc HubCargoTransfer::Move(VehicleCargoList &, StationCargoList &, uint, TileIndex) */
bool HubCargoTransfer::Move(VehicleCargoList &source, StationCargoList &target, uint quantity, TileIndex tile)
{
	if (!CanMove(source, quantity)) return false;
	// Unload the admitted Keep prefix without changing its permanent departure
	// evidence; returning to an original source can never restore refund rights.
	while (quantity != 0) {
		auto *packet = source.packets.front(); const uint count = std::min<uint>(quantity, packet->Count());
		auto *moved = count == packet->Count() ? packet : packet->Split(count); assert(moved != nullptr);
		source.RemoveFromMeta(moved, VehicleCargoList::MoveToAction::Keep, count);
		if (moved == packet) source.packets.pop_front();
		moved->UpdateUnloadingTile(tile); target.Append(moved, moved->GetNextHop()); quantity -= count;
	}
	return true;
}
/** @copydoc HubCargoTransfer::ReleaseOrbital */
bool HubCargoTransfer::ReleaseOrbital(VehicleCargoList &source, StationCargoList &target, uint quantity, TileIndex release_tile)
{
	// Only a real Space release into an empty descent owner may rebase the
	// physical travel vector, after whole ordinary native custody admission.
	if (GetTileLayer(release_tile) != MapLayer::Space || target.TotalCount() != 0 || !Move(source, target, quantity, release_tile)) return false;
	const int offset = TileY(release_tile) - TilePlaneY(release_tile);
	// Remove only the storage-plane offset from every released fragment. This
	// preserves actual travelled distance and creates no cross-layer income.
	for (StationCargoList::Iterator it(target.packets.begin()); it != target.packets.end(); ++it) {
		auto *packet = *it; packet->travelled.y = static_cast<int16_t>(packet->travelled.y + offset);
	}
	return true;
}
