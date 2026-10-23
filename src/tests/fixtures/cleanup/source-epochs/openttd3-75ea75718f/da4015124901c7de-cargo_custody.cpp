/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file cargo_custody.cpp Shared traversal factored from existing native asteroid provenance validation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md.
 */

#include "stdafx.h"
#include "cargo_custody.h"
#include "cargopacket.h"
#include "cargotype.h"
#include "station_base.h"
#include "vehicle_base.h"
#include "launch_center.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_structure.h"
#include "orbital_station.h"
#include "multilayer_space_content.h"
#include "safeguards.h"

/**
 * Inspect each routing bucket without changing the native station-style owner.
 * @param list Actual physical queue, compartment or job container.
 * @param cargo Container's declared registered physical meaning.
 * @param station Actual goods-queue identity, or invalid for compartments/jobs rather than a fabricated StationID.
 * @param visitor Consumer's read-only provenance/reference check.
 * @return Whether every actual reference passed.
 */
static bool VisitStationCargoPackets(const StationCargoList &list, CargoType cargo, StationID station, const NativeCargoPacketVisitor &visitor)
{
	// A routed fragment still belongs to this same physical owner. Inspect all
	// buckets so next-hop splits cannot conceal a duplicated or relabelled packet.
	for (const auto &[hop, packets] : *list.Packets()) {
		(void)hop;
		for (const CargoPacket *packet : packets) {
			// Missing native references must fail custody validation, never be
			// dereferenced or treated as an empty source capable of replacement stock.
			if (packet == nullptr || !visitor(*packet, cargo, station)) return false;
		}
	}
	return true;
}

/** @copydoc VisitNativeCargoPackets */
bool VisitNativeCargoPackets(const NativeCargoPacketVisitor &visitor)
{
	if (!visitor) return false;
	// Existing goods data alone owns station stock; inspection must not create
	// empty inventories. Handling jobs remain separate from their station's queue.
	for (const Station *station : Station::Iterate()) {
		for (CargoType cargo : EnumRange(NUM_CARGO)) {
			if (!station->goods[cargo].HasData()) continue;
			if (!VisitStationCargoPackets(station->goods[cargo].GetData().cargo, cargo, station->index, visitor)) return false;
		}
		if (station->launch_hub) {
			for (const auto &job : station->launch_hub->handling) {
				if (!VisitStationCargoPackets(job->packets, job->cargo, StationID::Invalid(), visitor)) return false;
			}
		}
	}
	// Freight stays in the native vehicle list; spacecraft supplies have their
	// own real typed compartments and never impersonate station stock.
	for (const Vehicle *vehicle : Vehicle::Iterate()) {
		for (const CargoPacket *packet : *vehicle->cargo.Packets()) {
			// A missing freight reference cannot borrow another packet's
			// provenance or paid rights simply because its vehicle still exists.
			if (packet == nullptr || !visitor(*packet, vehicle->cargo_type, StationID::Invalid())) return false;
		}
	}
	for (const SpaceVehicle *craft : SpaceVehicle::Iterate()) {
		if (!craft->manifest) continue;
		for (uint i = 0; i < SPACE_SUPPLY_COUNT; ++i) {
			if (!VisitStationCargoPackets(craft->manifest->supplies[i], GetSpaceSupplyCargo(static_cast<SpaceSupply>(i)), StationID::Invalid(), visitor)) return false;
		}
	}
	// A paid shell owns cargo only while its actual descent job exists.
	for (const SpacePod *pod : SpacePod::Iterate()) {
		if (!pod->descent) continue;
		if (!VisitStationCargoPackets(pod->descent->packets, pod->descent->cargo, StationID::Invalid(), visitor)) return false;
	}
	// Actual construction, completed storage and both service berths remain
	// distinct containers. No Core/Dock stock or reserved bill is counted twice.
	for (const SpaceStructure *site : SpaceStructure::Iterate()) {
		const auto *definition = GetSpaceConstructionDefinition(site->state.kind);
		if (definition == nullptr) return false;
		for (uint i = 0; i < 2; ++i) {
			if (!VisitStationCargoPackets(site->job.inputs[i], GetCargoTypeByLabel(definition->inputs[i].label), StationID::Invalid(), visitor)) return false;
		}
		if (!site->module) continue;
		const OrbitalModule &module = *site->module;
		for (CargoType cargo : EnumRange(NUM_CARGO)) {
			if (!VisitStationCargoPackets(module.stock[cargo], cargo, StationID::Invalid(), visitor)) return false;
		}
		for (const auto *job : module.GetServiceJobs()) {
			for (uint i = 0; i < 2; ++i) {
				if (!VisitStationCargoPackets(job->inputs[i], GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1)), StationID::Invalid(), visitor)) return false;
			}
		}
	}
	return true;
}
