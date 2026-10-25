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
bool ValidateSpaceCargoPreviewOwners(const LoadCheckData &data, SurfaceSupplyCustodyCheck *supply)
{
	// Every native owner family must come from this same checked file; a prior preview or active-world inventory cannot fill an absent table.
	if (!data.native_cargo_packets_present || !data.native_vehicle_cargo_present || !data.native_station_cargo_present) return false;
	std::set<CargoPacketID> owned;
	std::array<CargoLabel, NUM_CARGO> labels; labels.fill(CT_INVALID);
	if (supply != nullptr) {
		if (data.native_cargo_dimensions_present && !data.native_cargo_dimension_rows.empty()) { // Classic's original empty CDIM carries no configured meanings; its explicit vanilla climate remains the only available definition evidence.
			// Original configured meanings take precedence where this source
			// actually serialized them; repeated/undefined ordinals never default.
			for (const auto &row : data.native_cargo_dimension_rows) {
				if (row.cargo >= NUM_CARGO || row.label.Empty() || labels[row.cargo] != CT_INVALID) return false;
				labels[row.cargo] = row.label;
			}
		} else {
			const auto original = GetLegacyMultiLayerCargoLabels(static_cast<MultiLayerGameplayProfileID>(data.gameplay_profile_id),
				data.gameplay_profile_version, data.landscape, data.grfconfig);
			if (!original) return false;
			labels = *original;
		}
	}
	/** @param cargo Actual saved container slot. @return Candidate-owned configured/pinned meaning, never a receipt or live CargoSpec lookup. */
	const auto label = [&](CargoType cargo) { return cargo < NUM_CARGO ? labels[cargo] : CT_INVALID; };
	/** @param packet Actual same-file held identity. @param meaning Owner's declared cargo. @param station Original goods queue or Invalid. @return Whole optional shared custody agreement. */
	const auto admit = [&](CargoPacketID packet, CargoLabel meaning, StationID station = StationID::Invalid()) {
		if (supply == nullptr) return true;
		// Void native slots cannot supply physical material, even when another
		// receipt happens to name the desired construction supply.
		return meaning != CT_INVALID && !meaning.Empty() && supply->AddOwned(packet, meaning, station);
	};
	/** @param packet Original raw native packet ID. @return Whether its actual positive same-file row has not already financed another container. */
	auto add = [&](CargoPacketID packet) {
		const auto row = std::ranges::lower_bound(data.native_cargo_packet_rows, packet, {}, &NativeCargoPacketPreview::packet);
		return packet < CargoPacketID::End() && row != data.native_cargo_packet_rows.end() && row->packet == packet && row->quantity != 0 && owned.insert(packet).second;
	};
	/** @param references Original serialized SLE references, encoded as index+1. @param meaning Actual typed hull meaning, or empty for aggregate lists joined separately. @return Whether this complete native container has unique existing packets and any requested custody agreement. */
	auto native = [&](std::span<const uint32_t> references, CargoLabel meaning = {}) {
		// A null native reference is not a cargo packet. Decode only after admitting its original pool bound; no pointer or replacement packet is created.
		for (const auto reference : references) {
			if (reference == 0 || reference > CargoPacketID::End().base() || !add(CargoPacketID{reference - 1})) return false;
			if (!meaning.Empty() && !admit(CargoPacketID{reference - 1}, meaning)) return false; // Only a typed actual hull supplies this additional custody observation; aggregate native lists are joined separately below.
		}
		return true;
	};
	/** @param references Original feature container's raw packet/next-hop pairs. @param meaning Its actual declared/configured cargo label. @return Whether every physical reference remains exclusive and satisfies any requested paid custody across native/feature owners. */
	auto feature = [&](const auto &references, CargoLabel meaning) {
		// Next-hop routing never creates another inventory owner for a packet already held by a station, vehicle, module or paid job.
		for (const auto &[packet, hop] : references) { (void)hop; if (!add(packet) || !admit(packet, meaning)) return false; }
		return true;
	};
	// Ordinary stock and all actual carrier classes participate before Space jobs, closing aliases hidden outside the native Space subset.
	if (!native(data.native_station_cargo_refs) || !native(data.native_ordinary_vehicle_cargo_refs)) return false;
	if (supply != nullptr) {
		if (data.native_cargo_owner_rows.size() != owned.size()) return false;
		std::set<CargoPacketID> typed;
		// Aggregate native references and detailed saved queue/slot evidence
		// must describe exactly the same owners, once each, before Space joins.
		for (const auto &row : data.native_cargo_owner_rows) {
			if (row.reference == 0) return false;
			const CargoPacketID id{row.reference - 1};
			if (!owned.contains(id) || !typed.insert(id).second || !admit(id, label(row.cargo), row.station)) return false;
		}
	}
	for (const auto &row : data.native_space_vehicle_rows) if (!native(row.cargo_packet_refs, label(row.freight_cargo))) return false;
	// Surface handling has its own actual packet container; membership in a launch hub does not leave those same units in station stock.
	for (const auto &row : data.surface_handling_rows) if (!feature(row.packets, label(row.cargo))) return false;
	// Every manifest compartment remains separate from the hull's ordinary freight and from all contained service reservations.
	for (const auto &row : data.space_manifest_rows) {
		for (size_t slot = 0; slot < row.supplies.size(); ++slot) if (!feature(row.supplies[slot], GetSpaceSupplyLabel(static_cast<SpaceSupply>(slot)))) return false;
	}
	// Blueprint material, operational stock, both service berths and optional manufacturing batches all join the same original packet exclusivity check.
	for (const auto &row : data.space_structure_rows) {
		const auto *definition = GetSpaceConstructionDefinition(row.state.kind);
		if (supply != nullptr && definition == nullptr) return false; // Typed paid-supply custody needs the real recipe; the existing identity-only join does not validate unrelated role metadata.
		for (size_t slot = 0; slot < row.inputs.size(); ++slot) {
			if (supply == nullptr) { if (!feature(row.inputs[slot], {})) return false; continue; }
			if (slot >= definition->inputs.size()) { if (!row.inputs[slot].empty()) return false; continue; }
			if (!feature(row.inputs[slot], definition->inputs[slot].label)) return false;
		}
		if (!row.module) continue;
		for (CargoType cargo : EnumRange(NUM_CARGO)) if (!feature(row.module->stock[cargo], label(cargo))) return false;
		for (const auto *job : row.module->GetServiceJobs()) {
			for (size_t slot = 0; slot < job->inputs.size(); ++slot) if (!feature(job->inputs[slot], GetSpaceSupplyLabel(static_cast<SpaceSupply>(slot + 1)))) return false;
		}
		if (row.module->manufacture) {
			const auto &job = *row.module->manufacture;
			const auto *recipe = GetSpaceVehicleConstruction(job.model);
			if (supply != nullptr && recipe == nullptr) return false; // Preserve the identity-only caller's existing scope while requiring the actual recipe for paid-custody labels.
			// Only declared recipe inputs are physical; spare saved slots cannot
			// hide cargo outside the shared global owner/custody check.
			for (size_t slot = 0; slot < job.inputs.size(); ++slot) {
				if (supply == nullptr) { if (!feature(job.inputs[slot], {})) return false; continue; }
				if (slot >= recipe->inputs.size()) { if (!job.inputs[slot].empty()) return false; continue; }
				if (!feature(job.inputs[slot], recipe->inputs[slot].label)) return false;
			}
		}
	}
	// Only the original descent job holds released freight; a paid shell or cached quantity cannot create another copy elsewhere.
	for (const auto &row : data.space_descent_rows) if (!feature(row.packets, label(row.cargo))) return false;
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
