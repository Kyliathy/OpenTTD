/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_cargo_preview.cpp Global copied cargo ownership joins the actual native and feature transport families.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CD
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10B; processor stock roles use same-file labels only.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12; hauler refits/capacity join original same-file physical packets.
 * The temporary reference set validates exclusivity; it cannot reserve, move, consume or create physical cargo.
 */
#include "../stdafx.h"
#include "space_cargo_preview.h"
#include "../fios.h"
#include "../multilayer_content.h"
#include "../multilayer_space_content.h"
#include "../orbital_recipe.h"
#include <set>
#include "../safeguards.h"

/** @copydoc ValidateSpaceCargoPreviewOwners */
bool ValidateSpaceCargoPreviewOwners(const LoadCheckData &data)
{
	// Every native owner family must come from this same checked file; a prior preview or active-world inventory cannot fill an absent table.
	if (!data.native_cargo_packets_present || !data.native_vehicle_cargo_present || !data.native_station_cargo_present) return false;
	std::set<CargoPacketID> owned;
	/** @param packet Original raw native packet ID. @return Whether its actual positive same-file row has not already financed another container. */
	auto add = [&](CargoPacketID packet) {
		const auto row = std::ranges::lower_bound(data.native_cargo_packet_rows, packet, {}, &NativeCargoPacketPreview::packet);
		return packet < CargoPacketID::End() && row != data.native_cargo_packet_rows.end() && row->packet == packet && row->quantity != 0 && owned.insert(packet).second;
	};
	/** @param references Original serialized SLE references, encoded as index+1. @return Whether this complete native container has unique existing packets. */
	auto native = [&](std::span<const uint32_t> references) {
		// A null native reference is not a cargo packet. Decode only after admitting its original pool bound; no pointer or replacement packet is created.
		for (const auto reference : references) if (reference == 0 || reference > CargoPacketID::End().base() || !add(CargoPacketID{reference - 1})) return false;
		return true;
	};
	/** @param references Original feature container's raw packet/next-hop pairs. @return Whether every physical reference remains exclusive across earlier native and feature owners. */
	auto feature = [&](const auto &references) {
		// Next-hop routing never creates another inventory owner for a packet already held by a station, vehicle, module or paid job.
		for (const auto &[packet, hop] : references) { (void)hop; if (!add(packet)) return false; }
		return true;
	};
	// Ordinary stock and all actual carrier classes participate before Space jobs, closing aliases hidden outside the native Space subset.
	if (!native(data.native_station_cargo_refs) || !native(data.native_ordinary_vehicle_cargo_refs)) return false;
	for (const auto &row : data.native_space_vehicle_rows) if (!native(row.cargo_packet_refs)) return false;
	// Surface handling has its own actual packet container; membership in a launch hub does not leave those same units in station stock.
	for (const auto &row : data.surface_handling_rows) if (!feature(row.packets)) return false;
	// Every manifest compartment remains separate from the hull's ordinary freight and from all contained service reservations.
	for (const auto &row : data.space_manifest_rows) for (const auto &slot : row.supplies) if (!feature(slot)) return false;
	// Blueprint material, operational stock, both service berths and optional manufacturing batches all join the same original packet exclusivity check.
	for (const auto &row : data.space_structure_rows) {
		for (const auto &slot : row.inputs) if (!feature(slot)) return false;
		if (!row.module) continue;
		for (const auto &slot : row.module->stock) if (!feature(slot)) return false;
		for (const auto *job : row.module->GetServiceJobs()) for (const auto &slot : job->inputs) if (!feature(slot)) return false;
		if (row.module->manufacture) for (const auto &slot : row.module->manufacture->inputs) if (!feature(slot)) return false;
	}
	// Only the original descent job holds released freight; a paid shell or cached quantity cannot create another copy elsewhere.
	for (const auto &row : data.space_descent_rows) if (!feature(row.packets)) return false;
	return true;
}

/** @copydoc ValidateSpaceCargoPreviewDimensions */
bool ValidateSpaceCargoPreviewDimensions(const LoadCheckData &data)
{
	if (!data.native_cargo_dimensions_present) return false;
	// Classic retains unrestricted legacy content; its empty family creates no Space definition or profile rights.
	if (data.gameplay_profile_id == to_underlying(MultiLayerGameplayProfileID::Classic)) return data.native_cargo_dimension_rows.empty();
	if (data.native_cargo_dimension_rows.empty()) return false;
	CargoType previous = INVALID_CARGO;
	// Canonical ordinal order makes each saved physical cargo resolve to one bounded configured definition, including arbitrary required-content weights.
	for (const auto &row : data.native_cargo_dimension_rows) {
		// Duplicate/out-of-order ordinals or absent labels cannot supply an independent physical mass definition.
		if (row.cargo >= NUM_CARGO || row.label.Empty() || (previous != INVALID_CARGO && row.cargo <= previous)) return false;
		previous = row.cargo;
	}
	// A genuine upgraded unknown-role descent keeps its original absent dimensions; loading must not fabricate a historical weight receipt.
	for (const auto &job : data.space_descent_rows) {
		if (job.sender_model == SPACE_DESCENT_LEGACY_SENDER_MODEL) continue;
		const auto row = std::ranges::lower_bound(data.native_cargo_dimension_rows, job.cargo, {}, &NativeCargoDimensionPreview::cargo);
		// A coherent change to receipt weight/mass still refuses when it disagrees with the independently saved configured definition.
		if (row == data.native_cargo_dimension_rows.end() || row->cargo != job.cargo || row->weight != job.unit_weight) return false;
	}
	const auto capabilities = GetMultiLayerVehicleCapabilities();
	const auto hauler = std::ranges::find(capabilities, uint16_t{17}, &LayerVehicleCapability::model_id);
	// A paid general hauler must carry only its declared physical cargo. Resolve its actual ordinal, capacity and packet quantities from this same candidate; the open world's refits and weights grant no admission.
	for (const auto &product : data.space_production_rows) {
		if (product.state.model != 17) continue;
		const auto hull = std::ranges::find(data.native_space_vehicle_rows, product.craft.vehicle, &NativeSpaceVehiclePreview::vehicle);
		if (hull == data.native_space_vehicle_rows.end()) return false;
		const auto meaning = std::ranges::lower_bound(data.native_cargo_dimension_rows, hull->freight_cargo, {}, &NativeCargoDimensionPreview::cargo);
		// Positive native headroom cannot authorize an undeclared cargo, an absent configured meaning or an invented capacity; GOODS crates retain their saved fractional-tonne weight.
		if (hauler == capabilities.end() || meaning == data.native_cargo_dimension_rows.end() || meaning->cargo != hull->freight_cargo || meaning->weight == 0 ||
			std::ranges::find(hauler->cargo_labels, meaning->label) == hauler->cargo_labels.end() ||
			hull->freight_capacity != GetMultiLayerSpaceBudget().probe_hold_sixteenths / meaning->weight) return false;
		uint64_t quantity = 0;
		// Each SLE reference names an actual same-file positive packet. Global ownership admission separately prevents duplicate owners; these quantities enforce the whole physical hold's mass.
		for (const auto reference : hull->cargo_packet_refs) {
			if (reference == 0) return false;
			const auto packet = std::ranges::lower_bound(data.native_cargo_packet_rows, CargoPacketID{reference - 1}, {}, &NativeCargoPacketPreview::packet);
			// A missing or empty row cannot fund the declared holding, even when a cached carrier total would fit.
			if (packet == data.native_cargo_packet_rows.end() || packet->packet != CargoPacketID{reference - 1} || packet->quantity == 0) return false;
			quantity += packet->quantity;
		}
		if (quantity > hull->freight_capacity) return false;
	}
	// Processor buffers resolve every retained stock label from this same file's configured metadata. The active world's CargoSpec table cannot authorize another file's ordinal or unrelated factory cargo.
	for (const auto &site : data.space_structure_rows) {
		if (!site.module || !IsOrbitalRecipeKind(site.state.kind)) continue;
		for (CargoType cargo : EnumRange(NUM_CARGO)) {
			const auto quantity = site.module->quantities[cargo]; if (quantity == 0) continue;
			const auto meaning = std::ranges::lower_bound(data.native_cargo_dimension_rows, cargo, {}, &NativeCargoDimensionPreview::cargo);
			if (meaning == data.native_cargo_dimension_rows.end() || meaning->cargo != cargo || quantity > GetOrbitalRecipeCargoCapacity(site.state.kind, meaning->label)) return false;
		}
	}
	return true;
}
