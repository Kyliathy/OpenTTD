/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file transport_tube_cargo.h Bounded transfer admission over the sole native station packet owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9 transfer policy.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs-multilayer-vehicles-and-structures.md I-3/I-6.
 */

#ifndef TRANSPORT_TUBE_CARGO_H
#define TRANSPORT_TUBE_CARGO_H

#include "cargo_type.h"
#include "station_type.h"
#include <optional>

class StationCargoList;

inline constexpr uint TUBE_TERMINAL_TRANSFER_CAPACITY = 1024; ///< Whole units per cargo at one joined native tube station; pending/reserved custody counts in place.

/**
 * Inspect remaining transfer space without creating a stock or reservation owner.
 * @param station Actual native loading station, shared by its separate tube/rail/road facilities.
 * @param cargo Exact canonical cargo; invalid cargo has no available transfer space at a tube station.
 * @param include_pending Count already staged transfers in its actual loading queue, including this consist's earlier parts.
 * @return Whole available units, or UINT_MAX for a station without a native tube berth.
 * Ordinary station semantics remain unchanged. Full or overfull tube buffers
 * retain incoming cargo aboard; existing stock is never truncated to make room.
 */
uint GetTubeStationTransferRoom(StationID station, CargoType cargo, bool include_pending = true);

/** Apply the same finite tube capacity only to the exact actual receiving queue.
 * @param station Native destination named by the original payment/handling owner.
 * @param cargo Exact native cargo slot; an invalid slot cannot identify a queue.
 * @param destination Actual native cargo list receiving the transfer, never another station's or a detached list.
 * @param include_pending Count staged native arrivals when reserving future room; false during the physical unload avoids counting them twice.
 * @return Available units (including zero when full), or no value when this queue is not this station's tube interchange.
 * No value preserves the caller's ordinary transfer semantics; it never grants tube room to an unrelated container.
 */
std::optional<uint32_t> GetTubeInterchangeTransferRoom(StationID station, CargoType cargo,
	const StationCargoList *destination, bool include_pending = false);

#endif /* TRANSPORT_TUBE_CARGO_H */
