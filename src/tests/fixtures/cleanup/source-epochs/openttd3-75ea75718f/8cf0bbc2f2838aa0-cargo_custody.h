/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file cargo_custody.h Read-only traversal of existing native packet owners; no inventory or owner registry is created.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */

#ifndef CARGO_CUSTODY_H
#define CARGO_CUSTODY_H

#include "cargo_type.h"
#include "station_type.h"
#include <functional>

struct CargoPacket;

/** Receives an actual packet, its container's actual cargo type and its real goods-queue StationID (invalid for other containers). */
using NativeCargoPacketVisitor = std::function<bool(const CargoPacket &, CargoType, StationID)>;

/**
 * Visit actual station goods/handling, vehicle freight/supplies, descent and orbital job/module/service containers.
 * @param visitor Read-only predicate; false stops traversal without changing any packet, owner, reference or quantity.
 * @return Whether a nonempty visitor accepted every actual owned reference and all container definitions were resolvable.
 * The caller validates its own provenance/purpose and uniqueness. A pool packet absent from these owners is never invented here.
 */
bool VisitNativeCargoPackets(const NativeCargoPacketVisitor &visitor);

#endif /* CARGO_CUSTODY_H */
