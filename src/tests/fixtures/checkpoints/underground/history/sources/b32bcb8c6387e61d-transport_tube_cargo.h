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

#endif /* TRANSPORT_TUBE_CARGO_H */
