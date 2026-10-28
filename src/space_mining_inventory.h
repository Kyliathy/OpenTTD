/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_inventory.h Atomic native finite-resource extraction participant.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group15; typed-source preparation and pre-allocation coordinate checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group10
 * Existing body, hull and CargoPacket owners retain all material, paid work and provenance.
 */
#ifndef SPACE_MINING_INVENTORY_H
#define SPACE_MINING_INVENTORY_H

#include "source_type.h"
#include "map_type.h"
#include <optional>

struct SpaceVehicle;
struct AsteroidBody;

/** @param body Actual or copied natural body. @return Exact full-width identity/mineral source, or refusal for an invalid ID/class. This producer is prepared for Group21; existing live packets retain their legacy encoding until that reader is available. */
std::optional<Source> PrepareAsteroidMiningSource(const AsteroidBody &body);
/** @param body Actual or copied original resource position. @return Its rounded production tile on the current actual Space plane, or refusal before any coordinate arithmetic, allocation or reserve debit can escape that plane. */
std::optional<TileIndex> GetSpaceMiningProductionTile(const AsteroidBody &body);

/** Commit one ready original native extraction batch, preserving all input owners on refusal.
 * @param craft Actual attached mining hull whose native work reached its canonical interval.
 * @param body Actual finite body exclusively leased to that same hull.
 * @param allocation_admitted Additional bounded admission veto; true still requires actual native packet-pool capacity.
 * @return Whether a positive original packet batch committed, reset work and released a full/depleted lease when due.
 */
bool CommitSpaceMiningExtraction(SpaceVehicle &craft, AsteroidBody &body, bool allocation_admitted = true);

#endif /* SPACE_MINING_INVENTORY_H */
