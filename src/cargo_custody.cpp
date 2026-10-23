/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file cargo_custody.cpp One complete native packet-owner traversal shared by provenance policies.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native physical ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-h-cargo-custody.md actual container inventory and non-owners.
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

/** Visit one actual reference only after its owner supplies a real registered cargo meaning.
 * @param packet Actual container reference, never recovered from pool membership.
 * @param cargo Container's original registered cargo slot.
 * @param station Actual goods queue, or Invalid for every other owner.
 * @param visitor Read-only policy; it does not acquire packet ownership.
 * @return Complete reference/meaning agreement and consumer acceptance.
 */
static bool VisitCargoPacket(const CargoPacket *packet, CargoType cargo, StationID station, const NativeCargoPacketVisitor &visitor)
{
	// Missing references or undefined physical meanings cannot borrow another
	// queue's label or a receipt's desired cargo to legitimize paid material.
	if (packet == nullptr || !IsValidCargoType(cargo) || CargoSpec::Get(cargo)->label.Empty()) return false;
	return visitor(*packet, cargo, station);
}

/** Traverse all real routing buckets in one native station-style physical container.
 * @param list Actual queue, compartment or contained job, not a copied ID vector.
 * @param cargo Cargo meaning supplied by the real container's declaration.
 * @param station Original goods-queue ID only; non-station owners use Invalid.
 * @param visitor Read-only consumer of each actual reference.
 * @return Complete visitation; empty unused cargo slots allocate no definitions or stock.
 */
static bool VisitStationCargoPackets(const StationCargoList &list, CargoType cargo, StationID station, const NativeCargoPacketVisitor &visitor)
{
	// Routing fragments remain in this one physical owner. Looking only at
	// aggregate totals or the first bucket could hide duplicate or relabelled units.
	for (const auto &[hop, packets] : *list.Packets()) {
		(void)hop;
		for (const auto *packet : packets) {
			if (!VisitCargoPacket(packet, cargo, station, visitor)) return false;
		}
	}
	return true;
}

/** @copydoc VisitNativeCargoPackets */
bool VisitNativeCargoPackets(const NativeCargoPacketVisitor &visitor)
{
	if (!visitor) return false;
	// Existing goods queues alone own waiting station cargo. A reservation
	// cache is not another queue, and handling jobs own their packets separately.
	for (const auto *station : Station::Iterate()) {
		for (CargoType cargo : EnumRange(NUM_CARGO)) {
			if (!station->goods[cargo].HasData()) continue;
			if (!VisitStationCargoPackets(station->goods[cargo].GetData().cargo, cargo, station->index, visitor)) return false;
		}
		if (!station->launch_hub) continue;
		for (const auto &job : station->launch_hub->handling) {
			// A missing retained job is corruption, not permission to ignore its
			// cargo or to report the parent station as refundable source custody.
			if (job == nullptr || !VisitStationCargoPackets(job->packets, job->cargo, StationID::Invalid(), visitor)) return false;
		}
	}
	// Every hull's native freight, including reserved Load-range packets and
	// Space freight, is visited once here. Manifests do not repeat that freight.
	for (const auto *vehicle : Vehicle::Iterate()) {
		for (const auto *packet : *vehicle->cargo.Packets()) {
			if (!VisitCargoPacket(packet, vehicle->cargo_type, StationID::Invalid(), visitor)) return false;
		}
	}
	// Fuel and service parts live in the spacecraft's actual typed compartments;
	// craft/passenger/pod references in its manifest are not further cargo owners.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		if (!craft->manifest) continue;
		for (size_t slot = 0; slot < SPACE_SUPPLY_COUNT; ++slot) {
			if (!VisitStationCargoPackets(craft->manifest->supplies[slot], GetSpaceSupplyCargo(static_cast<SpaceSupply>(slot)), StationID::Invalid(), visitor)) return false;
		}
	}
	// A passive paid shell holds material only in its actual descent job;
	// center/carrier shell references cannot grant a second inventory.
	for (const auto *pod : SpacePod::Iterate()) {
		if (!pod->descent) continue;
		if (!VisitStationCargoPackets(pod->descent->packets, pod->descent->cargo, StationID::Invalid(), visitor)) return false;
	}
	// Native construction, operational stock, both service berths and assembly
	// inputs are distinct owners. Recipe processing itself reuses module stock.
	for (const auto *site : SpaceStructure::Iterate()) {
		const auto *definition = GetSpaceConstructionDefinition(site->state.kind);
		// The physical construction owner has exactly its declared slots; an
		// unknown or incompatible recipe cannot assign labels by array position.
		if (definition == nullptr || definition->inputs.size() != site->job.inputs.size()) return false;
		for (size_t slot = 0; slot < site->job.inputs.size(); ++slot) {
			if (!VisitStationCargoPackets(site->job.inputs[slot], GetCargoTypeByLabel(definition->inputs[slot].label), StationID::Invalid(), visitor)) return false;
		}
		if (!site->module) continue;
		const auto &module = *site->module;
		for (CargoType cargo : EnumRange(NUM_CARGO)) {
			if (!VisitStationCargoPackets(module.stock[cargo], cargo, StationID::Invalid(), visitor)) return false;
		}
		for (const auto *job : module.GetServiceJobs()) {
			for (size_t slot = 0; slot < job->inputs.size(); ++slot) {
				if (!VisitStationCargoPackets(job->inputs[slot], GetSpaceSupplyCargo(static_cast<SpaceSupply>(slot + 1)), StationID::Invalid(), visitor)) return false;
			}
		}
		if (!module.manufacture) continue;
		const auto &job = *module.manufacture;
		const auto *recipe = GetSpaceVehicleConstruction(job.model);
		// A retained paid job needs its exact immutable recipe. Unknown models
		// or more inputs than the real owner can hold cannot invent cargo meanings.
		if (recipe == nullptr || recipe->inputs.size() > job.inputs.size()) return false;
		for (size_t slot = 0; slot < job.inputs.size(); ++slot) {
			if (slot >= recipe->inputs.size()) {
				if (!job.inputs[slot].Packets()->empty()) return false;
				continue;
			}
			if (!VisitStationCargoPackets(job.inputs[slot], GetCargoTypeByLabel(recipe->inputs[slot].label), StationID::Invalid(), visitor)) return false;
		}
	}
	return true;
}
