/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file cargo_custody.h Read-only traversal of actual native cargo owners, never another inventory.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native physical ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-h-cargo-custody.md exact owner inventory.
 */
#ifndef CARGO_CUSTODY_H
#define CARGO_CUSTODY_H

#include "cargo_type.h"
#include "station_type.h"
#include <functional>

struct CargoPacket;

/** Read one actual physical reference; false stops traversal without altering any owner.
 * @param packet Actual packet held by the visited native container.
 * @param cargo Container's resolved cargo slot, never inferred from a receipt or packet source.
 * @param station Actual station goods-queue ID, or Invalid for vehicles, compartments, handling and jobs.
 * @return Whether the consumer's read-only policy accepts this reference; uniqueness belongs to that policy.
 */
using NativeCargoPacketVisitor = std::function<bool(const CargoPacket &packet, CargoType cargo, StationID station)>;

/** Visit every existing native station, vehicle, Space compartment and contained job packet owner.
 * @param visitor Read-only consumer of actual references; an empty callback refuses before traversal.
 * @return Complete visitation or refusal for a null reference, unresolved nonempty cargo/recipe, unused occupied input or consumer error.
 * This never creates goods data, allocates gameplay objects, changes stock/caches/RNG, or invents custody from the packet pool.
 * A native reserved packet belongs only to its vehicle. Pod references and processing metadata do not duplicate actual inventories.
 */
bool VisitNativeCargoPackets(const NativeCargoPacketVisitor &visitor);

#endif /* CARGO_CUSTODY_H */
