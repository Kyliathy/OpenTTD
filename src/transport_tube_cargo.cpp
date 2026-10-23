/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_cargo.cpp Native tube interchange stock and pending-transfer capacity.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md layer-local station ownership.
 */

#include "stdafx.h"
#include "transport_tube_cargo.h"
#include "transport_tube.h"
#include "station_base.h"
#include "station_map.h"
#include "rail_map.h"
#include "vehicle_base.h"
#include "safeguards.h"

/** Sole read-only capacity calculation shared by staging and exact physical-destination adapters.
 * @param station Actual resolved native station; no cargo data is allocated by this observation.
 * @param cargo Native cargo whose queued and reserved stock occupies the berth.
 * @param include_pending Whether actual queued vehicle Transfer designations also reserve room.
 * @return Whole finite room or no tube applicability; full/overfull stock is never truncated.
 */
static std::optional<uint32_t> CalculateTubeTransferRoom(const Station &station, CargoType cargo, bool include_pending)
{
	bool tube_berth = false;
	// Inspect only this bounded native facility footprint at a cargo transaction,
	// never every world tile per tick. Joined conventional berths and holes in
	// the rectangle cannot impersonate the actual canonical tube capability.
	for (const TileIndex tile : station.train_station) {
		// Only installed membership in this exact station can impose a tube
		// bound on its shared rail/road stock; rectangle holes/foreign tiles cannot.
		if (station.TileBelongsToRailStation(tile) && IsTubeRailType(GetRailType(tile))) {
			tube_berth = true;
			break;
		}
	}
	if (!tube_berth) return std::nullopt;
	if (cargo >= NUM_CARGO) return 0;
	uint64_t occupied = station.goods[cargo].TotalCount();
	if (include_pending) {
		// Existing queued vehicles already own staged transfer packets. Count
		// that custody directly so two arrivals cannot sell the same free slot;
		// loading reservations remain included in the native station TotalCount.
		for (const Vehicle *front : station.loading_vehicles) {
			for (const Vehicle *part = front; part != nullptr; part = part->Next()) {
				if (part->cargo_type == cargo) occupied += part->cargo.ActionCount(VehicleCargoList::MoveToAction::Transfer);
			}
		}
	}
	return occupied >= TUBE_TERMINAL_TRANSFER_CAPACITY ? 0 : TUBE_TERMINAL_TRANSFER_CAPACITY - static_cast<uint>(occupied);
}

/** @copydoc GetTubeStationTransferRoom */
uint GetTubeStationTransferRoom(StationID station_id, CargoType cargo, bool include_pending)
{
	const auto *station = Station::GetIfValid(station_id);
	if (station == nullptr) return UINT_MAX;
	return CalculateTubeTransferRoom(*station, cargo, include_pending).value_or(UINT_MAX);
}

/** @copydoc GetTubeInterchangeTransferRoom */
std::optional<uint32_t> GetTubeInterchangeTransferRoom(StationID station_id, CargoType cargo,
	const StationCargoList *destination, bool include_pending)
{
	const auto *station = Station::GetIfValid(station_id);
	// Invalid IDs/labels, unallocated goods and another physical list cannot
	// borrow this station's bound. Observing applicability never creates stock.
	if (station == nullptr || cargo >= NUM_CARGO || !station->goods[cargo].HasData() ||
		&station->goods[cargo].GetData().cargo != destination) return std::nullopt;
	return CalculateTubeTransferRoom(*station, cargo, include_pending);
}
