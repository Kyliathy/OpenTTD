/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file hub_cargo.cpp Dedicated station/job packet transfers; no fictitious vehicle or new cargo. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CC paid source custody. */
#include "stdafx.h"
#include "hub_cargo.h"
#include "safeguards.h"

bool HubCargoTransfer::CanMove(const StationCargoList &source, uint quantity)
{
	if (quantity == 0 || quantity > source.AvailableCount()) return false;
	for (StationCargoList::ConstIterator it(source.packets.begin()); it != source.packets.end(); ++it) {
		const auto *packet = *it;
		if (quantity < packet->Count()) return CargoPacket::CanAllocateItem();
		quantity -= packet->Count();
		if (quantity == 0) return true;
	}
	return false;
}

/** @copydoc HubCargoTransfer::Move(StationCargoList &, StationCargoList &, uint) */
bool HubCargoTransfer::Move(StationCargoList &source, StationCargoList &target, uint quantity)
{
	// Transfer requires distinct owners and a complete native packet/split
	// preflight; a failed route/job cannot move or partially dispatch paid stock.
	if (&source == &target || !CanMove(source, quantity)) return false;
	// Move each actual routed fragment once. Only the moved units lose source
	// refund eligibility; a partial packet's remaining source units stay eligible.
	while (quantity != 0) {
		StationCargoList::Iterator it(source.packets.begin());
		auto *packet = *it;
		const auto next = it.GetKey();
		const uint count = std::min<uint>(quantity, packet->Count());
		auto *moved = count == packet->Count() ? packet : packet->Split(count);
		assert(moved != nullptr); // The pure preflight guarantees the only required allocation.
		source.RemoveFromCache(moved, count);
		if (moved == packet) source.packets.erase(it);
		moved->MarkCompanySupplyShipped();
		target.Append(moved, next);
		quantity -= count;
	}
	return true;
}

void HubCargoTransfer::Age(StationCargoList &cargo)
{
	for (StationCargoList::Iterator it(cargo.packets.begin()); it != cargo.packets.end(); ++it) {
		auto *packet = *it;
		if (packet->periods_in_transit == UINT16_MAX) continue;
		++packet->periods_in_transit;
		cargo.cargo_periods_in_transit += packet->Count();
	}
}

bool HubCargoTransfer::Consume(StationCargoList &source, uint quantity)
{
	if (quantity == 0 || quantity > source.AvailableCount()) return false;
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

bool HubCargoTransfer::CanMove(const VehicleCargoList &source, uint quantity)
{
	if (quantity == 0 || quantity > source.TotalCount() || source.ActionCount(VehicleCargoList::MoveToAction::Keep) != source.TotalCount()) return false;
	for (const auto *packet : source.packets) {
		if (quantity < packet->Count()) return CargoPacket::CanAllocateItem();
		quantity -= packet->Count(); if (quantity == 0) return true;
	}
	return false;
}
bool HubCargoTransfer::Move(StationCargoList &source, VehicleCargoList &target, uint quantity, TileIndex tile)
{
	if (target.ActionCount(VehicleCargoList::MoveToAction::Keep) != target.TotalCount() || !CanMove(source, quantity)) return false;
	while (quantity != 0) {
		StationCargoList::Iterator it(source.packets.begin()); auto *packet = *it; const auto next = it.GetKey();
		const uint count = std::min<uint>(quantity, packet->Count());
		auto *moved = count == packet->Count() ? packet : packet->Split(count); assert(moved != nullptr);
		source.RemoveFromCache(moved, count); if (moved == packet) source.packets.erase(it);
		moved->SetNextHop(next); moved->UpdateLoadingTile(tile); target.Append(moved); quantity -= count;
	}
	return true;
}
bool HubCargoTransfer::Move(VehicleCargoList &source, StationCargoList &target, uint quantity, TileIndex tile)
{
	if (!CanMove(source, quantity)) return false;
	while (quantity != 0) {
		auto *packet = source.packets.front(); const uint count = std::min<uint>(quantity, packet->Count());
		auto *moved = count == packet->Count() ? packet : packet->Split(count); assert(moved != nullptr);
		source.RemoveFromMeta(moved, VehicleCargoList::MoveToAction::Keep, count);
		if (moved == packet) source.packets.pop_front();
		moved->UpdateUnloadingTile(tile); target.Append(moved, moved->GetNextHop()); quantity -= count;
	}
	return true;
}
bool HubCargoTransfer::ReleaseOrbital(VehicleCargoList &source, StationCargoList &target, uint quantity, TileIndex release_tile)
{
	if (GetTileLayer(release_tile) != MapLayer::Space || target.TotalCount() != 0 || !Move(source, target, quantity, release_tile)) return false;
	const int offset = TileY(release_tile) - TilePlaneY(release_tile);
	for (StationCargoList::Iterator it(target.packets.begin()); it != target.packets.end(); ++it) {
		auto *packet = *it; packet->travelled.y = static_cast<int16_t>(packet->travelled.y + offset);
	}
	return true;
}
