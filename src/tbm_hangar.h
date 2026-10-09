/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_hangar.h Physical destination admission for native TBM storage and orders.
 * @see docs/zz-reach2/upgrades/2026-10/r2tbmf-tbm-fixes.md Group8
 * @see docs/zz-reach2/architecture/vehicles/archi-vehicles.md
 */

#ifndef TBM_HANGAR_H
#define TBM_HANGAR_H

#include "depot_type.h"
#include "tile_type.h"
#include "vehicle_type.h"

struct Train;

/** Existing physical storage owner; this descriptor is never a second order queue or saved cargo store. */
enum class TBMHangarKind : uint8_t { RailDepot, PortalMagazine };

/** Exact admission failure for a selected facility; route search still owns reachability after admission. */
enum class TBMHangarError : uint8_t { None, InvalidMachine, InvalidDestination, WrongOwner, IncompatibleRail, LockedLayer, UnfinishedPortal, Occupied };

/** Read-only storage description; only genuine native depots supply a DepotID. */
struct TBMHangarInfo {
	TBMHangarError error = TBMHangarError::InvalidDestination; ///< No authority on refusal.
	TBMHangarKind kind = TBMHangarKind::RailDepot; ///< Actual existing storage geometry.
	TileIndex tile = INVALID_TILE; ///< Canonical physical storage tile, including the upper housing for a portal pair.
	DepotID depot = DepotID::Invalid(); ///< Existing native pool identity; a portal needs registration by the order command before use as a depot order.
	VehicleID blocker = VehicleID::Invalid(); ///< Actual occupying train when it can be identified.
};

/**
 * Describe an existing compatible hangar without changing trains, orders, jobs, reservations or infrastructure.
 * Native rail depots retain their normal storage capacity. A portal magazine reserves one cutter's storage;
 * only that cutter's assigned exchange hauler may share its existing bay during withdrawal and settlement.
 * A legal incoming path remains a separate native pathfinder requirement; this query creates no track or cavity.
 * @param front Actual native cutter front, assigned or retained after project release.
 * @param destination Clicked depot or either endpoint of a rail portal pair.
 * @return Physical storage identity and current admission, with a real blocking vehicle where available.
 */
TBMHangarInfo DescribeTBMHangar(const Train &front, TileIndex destination);

/**
 * Obtain the ordinary depot identity for an admitted destination, registering existing portal storage if needed.
 * Call only at synchronized command execution after all order/controller preparation has succeeded.
 * This adds no geometry, vehicles, cargo or capacity. Estimates must use DescribeTBMHangar instead.
 * @param front Actual native cutter whose facility admission is revalidated.
 * @param destination Real depot or either portal endpoint.
 * @return The actual pool ID, or Invalid with no mutation on admission/pool-capacity refusal.
 */
DepotID RegisterTBMHangarDepot(const Train &front, TileIndex destination);

/** @param front Actual cutter. @return Whether its native current order requests a depot stop. */
bool HasTBMHangarHaltOrder(const Train &front);
/** @param front Actual cutter. @return Whether its native destination is deferred until real spoil exchange. */
bool IsTBMStagingHangarOrder(const Train &front);
/** @param front Actual cutter. @return Current native depot destination admission; no order is invented on absence. */
TBMHangarInfo GetTBMOrderedHangar(const Train &front);
/** @param front Actual cutter. @return Whether every part is stationary, empty and stored in one compatible real hangar. */
bool IsTBMWholeInHangar(const Train &front);
/**
 * Admit this real cutter through its ordered portal, or finish leaving storage it physically occupies.
 * @param front Actual front engine; its current order and actual part positions supply authority.
 * @param tile Either physical endpoint being queried by movement/pathfinding.
 * @return Whether this specific owned registered portal may carry the train; never opens ground or public routing.
 */
bool CanTBMUseHangarPortal(const Train &front, TileIndex tile);
/**
 * Suspend work for a native halt order, or let a new service order replace an unfinished hangar trip.
 * @param front Actual cutter with an already published native order; caller owns command/tick synchronization.
 * @return Whether an existing controller adopted the intent and selected a native bay-return direction; no geological or order fields are changed.
 */
bool BeginTBMHangarReturn(Train &front);
/**
 * Record actual whole-consist arrival before native depot completion consumes its current order.
 * @param front Actual arriving native cutter, still carrying its matching depot halt order.
 * @return Whether a travelling work controller became parked; never releases the geological job.
 */
bool CompleteTBMHangarArrival(const Train &front);

#endif /* TBM_HANGAR_H */
