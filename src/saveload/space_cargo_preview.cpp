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
#include "../asteroid_cargo.h"
#include <set>
#include <map>
#include "../safeguards.h"

/** @param rows Same-file configured cargo slots in serialized order. @return Whether binary label lookup is safe without normalizing corrupt ordering or consulting live CargoSpec. */
static bool ValidateCargoMeaningRows(std::span<const NativeCargoDimensionPreview> rows)
{
	CargoType previous = INVALID_CARGO;
	// One actual configured meaning must exist for each retained slot. Empty,
	// duplicate or descending rows cannot authorize a natural packet's mineral.
	for (const auto &row : rows) {
		if (row.cargo >= NUM_CARGO || row.label.Empty() || (previous != INVALID_CARGO && row.cargo <= previous)) return false;
		previous = row.cargo;
	}
	return true;
}

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
	// Allocated asteroid freight must have one actual candidate owner. Orphans
	// cannot become historical reserve merely because their source body retired.
	for (const auto &packet : data.native_cargo_packet_rows) {
		if (!packet.source.IsCanonical() || (packet.source.type == SourceType::Asteroid && !owned.contains(packet.packet))) return false;
	}
	return ValidateSpaceAsteroidCargoPreview(data);
}

/** @copydoc ValidateSpaceAsteroidCargoPreview */
bool ValidateSpaceAsteroidCargoPreview(const LoadCheckData &data)
{
	// Independent family validators own ordinary recipe/role admission. This
	// historical-origin join has no additional requirements when no natural
	// packets exist; ownership-only fixtures need no fabricated asteroid field.
	/** @param row Original copied packet metadata. @return Whether its natural domain requires this historical-origin join. */
	const auto is_asteroid = [](const auto &row) { return row.source.type == SourceType::Asteroid; };
	if (std::ranges::none_of(data.native_cargo_packet_rows, is_asteroid)) return true;
	// Both binary lookup inputs must be this candidate's bounded, ordered rows;
	// a malformed optional family cannot substitute another file's meaning.
	if ((!data.native_cargo_dimensions_present && !data.native_cargo_dimension_rows.empty()) ||
		!ValidateCargoMeaningRows(data.native_cargo_dimension_rows) ||
		!ValidateAsteroidIdentityRecords(data.asteroid_body_rows, data.asteroid_next_body_id)) return false;
	std::map<uint32_t, CargoType> ordinary_meanings;
	// This temporary join contains only original ordinary reference meanings;
	// it cannot allocate packets or supply labels from an active CargoSpec table.
	for (const auto &row : data.native_cargo_reference_rows) {
		if (row.reference == 0 || row.reference > CargoPacketID::End().base() || row.cargo >= NUM_CARGO ||
			!ordinary_meanings.emplace(row.reference, row.cargo).second) return false;
	}
	std::set<uint32_t> used_meanings;
	/** @param cargo Same-file container slot. @return Same-file configured meaning, or explicit absence in old/incomplete metadata. */
	auto meaning = [&](CargoType cargo) -> std::optional<CargoLabel> {
		const auto found = std::ranges::lower_bound(data.native_cargo_dimension_rows, cargo, {}, &NativeCargoDimensionPreview::cargo);
		return found != data.native_cargo_dimension_rows.end() && found->cargo == cargo ? std::optional{found->label} : std::nullopt;
	};
	/** @param id Original packet ID. @param label Container's actual same-file or immutable recipe label. @return Whether its natural origin/production geometry agrees, without conferring any ownership beyond the separate global join. */
	auto packet = [&](CargoPacketID id, std::optional<CargoLabel> label) {
		const auto found = std::ranges::lower_bound(data.native_cargo_packet_rows, id, {}, &NativeCargoPacketPreview::packet);
		// Every referenced original packet needs a real row before its source can
		// be classified; no missing row is interpreted as ordinary harmless cargo.
		if (found == data.native_cargo_packet_rows.end() || found->packet != id || !found->source.IsCanonical()) return false;
		if (found->source.type != SourceType::Asteroid) return true;
		const auto origin = ResolveAsteroidCargoOrigin(found->source, data.asteroid_body_rows, data.asteroid_next_body_id);
		const auto bounds = GetSpacePlaneBounds(data.map_size_x, data.map_size_y);
		// Production belongs to this file's Space plane and its original no-first-
		// station receipt. Preview checks geometry only; full loading checks tile type.
		if (!origin || !data.asteroid_present || !data.asteroid_active || !bounds || data.map_layers <= to_underlying(MapLayer::Space) ||
			found->first_station != StationID::Invalid()) return false;
		const uint64_t plane = uint64_t{data.map_size_x} * data.map_size_y;
		if (uint64_t{found->source_tile.base()} / plane != to_underlying(MapLayer::Space)) return false;
		if (label) return *label == origin->mineral;
		// Old source eras had no configured-dimension family. Preserve their
		// native-only ordinal check without guessing from the open world's cargo;
		// a modern typed origin or present-but-missing CDIM row cannot use it.
		return (!found->source.GetAsteroidOrigin() || found->legacy_asteroid_origin) && !data.native_cargo_dimensions_present;
	};
	/** @param references Encoded native IDs+1. @param label Known Space-hull cargo meaning, or absence for ordinary rows using their captured meanings. @param ordinary Whether these references use the ordinary station/carrier meaning join. @return Whether all original packet meanings validate. */
	auto native = [&](std::span<const uint32_t> references, std::optional<CargoLabel> label, bool ordinary) {
		for (const auto reference : references) {
			if (reference == 0 || reference > CargoPacketID::End().base()) return false;
			auto expected = label;
			if (ordinary) {
				const auto found = ordinary_meanings.find(reference);
				if (found != ordinary_meanings.end()) {
					used_meanings.insert(reference);
					expected = meaning(found->second);
				}
			}
			if (!packet(CargoPacketID{reference - 1}, expected)) return false;
		}
		return true;
	};
	/** @param references Original feature packet/next-hop pairs. @param label That owner's actual same-file or immutable recipe label. @return Whether each existing packet carries compatible natural meaning. */
	auto feature = [&](const auto &references, std::optional<CargoLabel> label) {
		for (const auto &[id, hop] : references) { (void)hop; if (!packet(id, label)) return false; }
		return true;
	};
	// Ordinary station/carrier packets use captured slot meanings. Space hulls,
	// handling and descent records already carry their own original cargo slot.
	if (!native(data.native_station_cargo_refs, std::nullopt, true) || !native(data.native_ordinary_vehicle_cargo_refs, std::nullopt, true)) return false;
	if (used_meanings.size() != ordinary_meanings.size()) return false;
	for (const auto &row : data.native_space_vehicle_rows) if (!native(row.cargo_packet_refs, meaning(row.freight_cargo), false)) return false;
	for (const auto &row : data.surface_handling_rows) if (!feature(row.packets, meaning(row.cargo))) return false;
	for (const auto &row : data.space_descent_rows) if (!feature(row.packets, meaning(row.cargo))) return false;
	// Fixed supply/recipe labels are immutable definitions, not runtime cargo
	// ordinals. Every physical reservation remains with its original container.
	for (const auto &row : data.space_manifest_rows) for (uint slot = 0; slot < row.supplies.size(); ++slot)
		if (!feature(row.supplies[slot], GetSpaceSupplyLabel(static_cast<SpaceSupply>(slot)))) return false;
	for (const auto &row : data.space_structure_rows) {
		const auto *construction = GetSpaceConstructionDefinition(row.state.kind);
		if (construction == nullptr) return false;
		for (uint slot = 0; slot < row.inputs.size(); ++slot)
			if (!feature(row.inputs[slot], construction->inputs[slot].label)) return false;
		if (!row.module) continue;
		for (CargoType cargo : EnumRange(NUM_CARGO)) if (!feature(row.module->stock[cargo], meaning(cargo))) return false;
		for (const auto *job : row.module->GetServiceJobs()) for (uint slot = 0; slot < job->inputs.size(); ++slot)
			if (!feature(job->inputs[slot], GetSpaceSupplyLabel(static_cast<SpaceSupply>(slot + 1)))) return false;
		if (row.module->manufacture) {
			const auto &job = *row.module->manufacture;
			const auto *recipe = GetSpaceVehicleConstruction(job.model);
			if (recipe == nullptr || recipe->inputs.size() > job.inputs.size()) return false;
			for (uint slot = 0; slot < job.inputs.size(); ++slot) {
				if (slot >= recipe->inputs.size()) { if (!job.inputs[slot].empty()) return false; continue; }
				if (!feature(job.inputs[slot], recipe->inputs[slot].label)) return false;
			}
		}
	}
	return true;
}

/** @copydoc NormalizeLegacyAsteroidCargoPreview */
bool NormalizeLegacyAsteroidCargoPreview(LoadCheckData &data)
{
	std::vector<std::pair<size_t, Source>> upgrades;
	/** @param row Original copied packet. @return Whether this family needs the same-file natural-origin migration. */
	const auto natural = [](const auto &row) { return row.source.type == SourceType::Asteroid; };
	if (std::ranges::none_of(data.native_cargo_packet_rows, natural)) return true;
	// Validate the candidate identity set once before binary per-packet joins;
	// old no-CDIM packets still require their own complete active field.
	if (!data.asteroid_present || !data.asteroid_active || !ValidateAsteroidIdentityRecords(data.asteroid_body_rows, data.asteroid_next_body_id)) return false;
	// Resolve every original old packet using only this candidate's exact body
	// metadata. A failure cannot leave a partially normalized copied family.
	for (size_t index = 0; index < data.native_cargo_packet_rows.size(); ++index) {
		const auto &row = data.native_cargo_packet_rows[index];
		if (row.source.type != SourceType::Asteroid) continue;
		const auto origin = ResolveAsteroidCargoOrigin(row.source, data.asteroid_body_rows, data.asteroid_next_body_id);
		if (!origin) return false;
		if (row.source.GetAsteroidOrigin()) {
			if (!row.legacy_asteroid_origin) return false;
			continue; // Only this explicit migration receipt makes a repeated old-file normalization idempotent.
		}
		upgrades.emplace_back(index, Source::FromAsteroid(origin->body, origin->mineral));
	}
	// Publish detached values only after the complete source join. The legacy
	// receipt retains genuine old no-CDIM admission without making that fallback
	// available to any modern decoder or allocating a native packet.
	for (const auto &[index, source] : upgrades) {
		auto &row = data.native_cargo_packet_rows[index]; row.source = source; row.legacy_asteroid_origin = true;
	}
	return true;
}

/** @copydoc ValidateSpaceCargoPreviewDimensions */
bool ValidateSpaceCargoPreviewDimensions(const LoadCheckData &data)
{
	if (!data.native_cargo_dimensions_present) return false;
	// Classic retains unrestricted legacy content; its empty family creates no Space definition or profile rights.
	if (data.gameplay_profile_id == to_underlying(MultiLayerGameplayProfileID::Classic)) return data.native_cargo_dimension_rows.empty();
	if (data.native_cargo_dimension_rows.empty()) return false;
	if (!ValidateCargoMeaningRows(data.native_cargo_dimension_rows)) return false;
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
