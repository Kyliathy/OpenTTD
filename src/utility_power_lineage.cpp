/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_power_lineage.cpp Pure bounded parent/physical-segment conservation and explicit older one-hop migration.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group8C
 */

#include "stdafx.h"
#include "utility_power_lineage.h"
#include <set>
#include "safeguards.h"

/** Borrowed per-call index into the sole shared owner; it never stores another resource balance. */
struct UtilityLineageNode {
	const UtilityPowerShare *share = nullptr; ///< Exact retained slice with its immutable epoch identity.
	const UtilityPowerConsumer *consumer = nullptr; ///< Actual reservation containing this slice.
};

/** @param tile Saved explicit world address. @param layer Declared native plane. @param geometry This file's copied dimensions. @return Whether its address lies on the declared plane, without consulting a live map. */
static bool IsUtilityLineageTile(TileIndex tile, MapLayer layer, UtilityLineageGeometry geometry)
{
	if (geometry.width == 0) return true;
	const uint64_t plane = static_cast<uint64_t>(geometry.width) * geometry.height;
	return tile != INVALID_TILE && to_underlying(layer) < geometry.layers && tile.base() / plane == to_underlying(layer);
}

/** @param first Original saved cell. @param second Proposed cardinal neighbor. @param geometry Explicit copied dimensions. @return Whether the cells adjoin within one plane; this decodes file geometry and defines no gameplay tile arithmetic. */
static bool AreUtilityLineageNeighbors(TileIndex first, TileIndex second, UtilityLineageGeometry geometry)
{
	if (geometry.width == 0) return first != second;
	const uint64_t plane = static_cast<uint64_t>(geometry.width) * geometry.height;
	if (first.base() / plane != second.base() / plane) return false;
	const uint32_t a = static_cast<uint32_t>(first.base() % plane), b = static_cast<uint32_t>(second.base() % plane);
	const auto dx = static_cast<int64_t>(a % geometry.width) - b % geometry.width;
	const auto dy = static_cast<int64_t>(a / geometry.width) - b / geometry.width;
	return std::abs(dx) + std::abs(dy) == 1;
}

/** @copydoc ValidateUtilityPowerLineage */
UtilityPowerError ValidateUtilityPowerLineage(const UtilityPowerState &state, UtilityLineageGeometry geometry)
{
	// Malformed copied geometry cannot disable just one axis or overflow an
	// address. Omitting both axes is reserved for pure accounting fixtures.
	if ((geometry.width == 0) != (geometry.height == 0) || geometry.width > 4096 || geometry.height > 4096 || geometry.layers > MAX_MAP_LAYERS ||
		(geometry.width != 0 && (geometry.width < 64 || geometry.height < 64))) return UtilityPowerError::InvalidTopology;
	if (state.next_share > MAX_UTILITY_POWER_SHARES) return UtilityPowerError::Limit;
	std::vector<UtilityLineageNode> nodes(state.next_share);
	size_t count = 0;
	// Every retained slice has one dense shared identity across both domains.
	// Indexing by it survives canonical presentation sorting and resolves an
	// exact parent, rather than guessing from a producer or final route.
	for (size_t index = 0; index < state.consumers.size(); ++index) {
		const auto &consumer = state.consumers[index];
		if (consumer.meter.consumer_id.base() != index) return UtilityPowerError::InvalidReference;
		for (const auto &share : consumer.shares) {
			if (++count > MAX_UTILITY_POWER_SHARES || share.id.base() >= nodes.size() || nodes[share.id.base()].share != nullptr || share.producer.base() >= state.producers.size()) return UtilityPowerError::InvalidReference;
			if (share.allocated_j == 0 || share.consumed_j > share.allocated_j || share.released_j > share.allocated_j - share.consumed_j ||
				share.transferred_j > share.allocated_j - share.consumed_j - share.released_j) return UtilityPowerError::InvalidQuantity; // Every independent settlement fits the original slice before any free-balance subtraction.
			nodes[share.id.base()] = {&share, &consumer};
		}
	}
	if (count != nodes.size()) return UtilityPowerError::InvalidReference;
	std::vector<uint64_t> children(nodes.size());
	std::vector<std::vector<UtilityLinkID>> paths(nodes.size());
	std::vector<UtilityConsumerID> roots(nodes.size(), UtilityConsumerID{UINT32_MAX});
	std::map<UtilityLinkID, uint64_t> flows;
	std::map<UtilityLinkID, uint32_t> durations;
	std::map<UtilityProducerID, uint64_t> original_allocations;
	size_t references = 0;
	// Parent-before-child identities allow one bounded forward audit. Full
	// paths below are temporary references, capped across the entire graph;
	// original generation and free quantities remain in their shared slices.
	for (size_t index = 0; index < nodes.size(); ++index) {
		const auto &[slice, consumer] = nodes[index]; const auto &share = *slice;
		const auto &producer = state.producers[share.producer.base()];
		if (producer.meter.producer_id != share.producer || producer.owner != consumer->owner || producer.meter.domain != consumer->meter.domain) return UtilityPowerError::InvalidReference; // Parent/child identities cannot substitute another company's or domain's original generation.
		if (share.parent.base() == UINT32_MAX) {
			if (!share.segment.empty() || share.imported_link.base() != UINT32_MAX || producer.meter.layer != consumer->meter.layer ||
				(consumer->meter.domain == UtilityEnergyDomain::Heat && share.transferred_j != 0)) return UtilityPowerError::InvalidTopology; // Original roots are local; Heat never acquires an electrical import or outgoing child.
			// Released root promises still came from this original publication.
			// They cannot conceal a second root allocation whose free balance
			// merely happens to fit the source's remaining numerical reserve.
			if (original_allocations[share.producer] > producer.meter.generation_energy_j ||
				share.allocated_j > producer.meter.generation_energy_j - original_allocations[share.producer]) return UtilityPowerError::InvalidQuantity;
			original_allocations[share.producer] += share.allocated_j;
			roots[index] = consumer->meter.consumer_id; continue;
		}
		// Earlier parents rule out cycles before traversal or arithmetic. A
		// child names the exact preceding slice and original generation owner.
		if (share.parent.base() >= index || share.segment.empty() || consumer->meter.domain != UtilityEnergyDomain::Electricity) return UtilityPowerError::InvalidTopology;
		const auto &parent = nodes[share.parent.base()];
		if (parent.share->producer != share.producer || parent.consumer->owner != consumer->owner || parent.consumer->meter.domain != consumer->meter.domain) return UtilityPowerError::InvalidTopology;
		if (share.allocated_j > parent.share->transferred_j - children[share.parent.base()]) return UtilityPowerError::InvalidQuantity;
		children[share.parent.base()] += share.allocated_j;
		if (paths[share.parent.base()].size() + share.segment.size() > MAX_UTILITY_INTERVAL_ACCOUNTS ||
			references + paths[share.parent.base()].size() + share.segment.size() > MAX_UTILITY_LINEAGE_LINKS) return UtilityPowerError::Limit; // A long parent chain cannot hide unbounded reconstructed path storage or repeated validation work.
		paths[index] = paths[share.parent.base()]; roots[index] = roots[share.parent.base()];
		std::set<UtilityLinkID> visited(paths[index].begin(), paths[index].end());
		std::set<UtilityConsumerID> accounts{roots[index]};
		const uint64_t free = share.allocated_j - share.consumed_j - share.released_j - share.transferred_j;
		// An ancestor's lost physical reference invalidates all unused descendant
		// availability, even when the final arriving edge remains connected.
		for (const auto id : paths[index]) {
			if (id.base() >= state.links.size()) return UtilityPowerError::InvalidReference;
			const auto &link = state.links[id.base()];
			if (link.kind == UtilityLinkKind::ElectricalPortal || link.kind == UtilityLinkKind::ElectricalFeed) return UtilityPowerError::InvalidTopology; // Generic imports and terminal local feeds cannot be re-exported through a later route.
			if (!link.active && free != 0) return UtilityPowerError::InvalidQuantity;
			if (link.kind != UtilityLinkKind::LocalCell) accounts.insert(link.destination);
		}
		if (!parent.consumer->active && free != 0) return UtilityPowerError::InvalidQuantity;
		UtilityConsumerID current = parent.consumer->meter.consumer_id;
		std::optional<TileIndex> expected;
		if (!paths[index].empty()) expected = state.links[paths[index].back().base()].destination_endpoint;
		std::optional<TileIndex> last_cell;
		UtilityLinkID incoming{UINT32_MAX};
		// Every newly traversed segment is explicit installed cable followed by
		// an original paid transition. Historical generic transfers keep their
		// exact one-edge role and never become a re-export fallback.
		for (const auto id : share.segment) {
			if (id.base() >= state.links.size() || !visited.insert(id).second) return UtilityPowerError::InvalidTopology;
			const auto &link = state.links[id.base()];
			if (link.id != id || (!link.active && free != 0)) return UtilityPowerError::InvalidReference;
			if (link.kind == UtilityLinkKind::LocalCell) {
				if (link.local_domain != UtilityEnergyDomain::Electricity || link.local_owner != consumer->owner || current.base() >= state.consumers.size() ||
					link.local_layer != state.consumers[current.base()].meter.layer || !IsUtilityLineageTile(link.source_endpoint, link.local_layer, geometry)) return UtilityPowerError::InvalidTopology; // Each cable remains on the actual current electrical plane and controller.
				if (last_cell.has_value()) {
					if (!AreUtilityLineageNeighbors(*last_cell, link.source_endpoint, geometry)) return UtilityPowerError::InvalidTopology;
				} else if (expected.has_value() && link.source_endpoint != *expected) return UtilityPowerError::InvalidTopology; // An arriving housing cannot teleport to another inlet through an equal account ID.
				last_cell = link.source_endpoint;
			} else {
				if (link.source != current || link.destination.base() >= state.consumers.size() || !accounts.insert(link.destination).second ||
					state.consumers[link.destination.base()].owner != consumer->owner || state.consumers[link.destination.base()].meter.domain != UtilityEnergyDomain::Electricity) return UtilityPowerError::InvalidTopology; // Exact directed endpoints and distinct accounts prevent foreign or circulating imports.
				if (link.kind == UtilityLinkKind::ElectricalPortal) {
					if (share.segment.size() != 1 || !paths[share.parent.base()].empty() || parent.share->parent.base() != UINT32_MAX) return UtilityPowerError::InvalidTopology; // Preserve genuine older one-hop semantics; generic imports cannot be extended or mixed into paid relay paths.
				} else if (link.kind == UtilityLinkKind::ElectricalRiser) {
					if (!last_cell.has_value() || !AreUtilityLineageNeighbors(*last_cell, link.source_endpoint, geometry)) return UtilityPowerError::InvalidTopology;
				} else if (link.kind == UtilityLinkKind::ElectricalFeed) {
					// One explicit native plant root supplies a complete local cable
					// ending at the recipient. No imported parent or later leg is legal.
					if (parent.share->parent.base() != UINT32_MAX || incoming.base() != UINT32_MAX || !last_cell.has_value() ||
						*last_cell != link.destination_endpoint || id != share.segment.back() ||
						state.links[share.segment.front().base()].source_endpoint != link.source_endpoint ||
						producer.kind != UtilityGenerationKind::HeatConversion || parent.consumer->linked_producers.size() > 1 ||
						(link.active && parent.consumer->linked_producers != std::vector{share.producer})) return UtilityPowerError::InvalidTopology;
				} else return UtilityPowerError::InvalidTopology;
				if (!IsUtilityLineageTile(link.source_endpoint, state.consumers[current.base()].meter.layer, geometry) ||
					!IsUtilityLineageTile(link.destination_endpoint, state.consumers[link.destination.base()].meter.layer, geometry)) return UtilityPowerError::InvalidTopology; // Copied endpoint addresses must agree with their own file's explicit planes.
				current = link.destination; expected = link.destination_endpoint; last_cell.reset(); incoming = id;
			}
			uint64_t limit = link.kind == UtilityLinkKind::ElectricalFeed ? MAX_UTILITY_ACCOUNT_ENERGY_J : link.capacity_j;
			if (link.kind != UtilityLinkKind::ElectricalPortal && link.kind != UtilityLinkKind::ElectricalFeed) {
				const auto integrated = IntegrateUtilityGeneration(link.kind == UtilityLinkKind::LocalCell ? link.local_capacity_w : link.riser_capacity_w, producer.meter.economy_ticks, 0);
				if (!integrated.has_value() || producer.meter.economy_ticks == 0) return UtilityPowerError::InvalidInterval; // Physical opportunity belongs to the original published synchronized window, never a caller's quote.
				limit = integrated->energy_j;
				const auto [period, inserted] = durations.emplace(id, producer.meter.economy_ticks);
				if (!inserted && period->second != producer.meter.economy_ticks) return UtilityPowerError::InvalidInterval; // One physical resource cannot acquire an independent interval capacity for each source's different window.
			}
			if (flows[id] > limit || share.allocated_j > limit - flows[id]) return UtilityPowerError::InvalidQuantity; // All paths and original sources spend one shared physical interval allowance; a second parent cannot multiply it.
			flows[id] += share.allocated_j; paths[index].push_back(id);
		}
		if (last_cell.has_value() || current != consumer->meter.consumer_id || incoming != share.imported_link) return UtilityPowerError::InvalidTopology; // A retained import ends at its requested destination and final native edge, never an unfinished local cable.
		references += paths[index].size();
	}
	// Each direct donor debit has exactly one conserved child allocation. A
	// descendant's later segment never recounts its ancestor's physical flow.
	for (size_t index = 0; index < nodes.size(); ++index) {
		if (children[index] != nodes[index].share->transferred_j) return UtilityPowerError::InvalidQuantity;
	}
	for (const auto &link : state.links) {
		if (link.kind != UtilityLinkKind::LocalCell && link.flow_j != flows[link.id]) return UtilityPowerError::InvalidQuantity;
	}
	return UtilityPowerError::None;
}

/** @copydoc MigrateUtilityPowerLineage */
UtilityPowerError MigrateUtilityPowerLineage(UtilityPowerState &state)
{
	if (state.next_share != 0) return ValidateUtilityPowerLineage(state);
	UtilityPowerState migrated = state;
	std::map<std::pair<UtilityConsumerID, UtilityProducerID>, UtilityShareID> roots;
	// Number original roots first in canonical consumer/source order. Genuine
	// older imports are one-hop only, so every parent then already exists.
	for (auto &consumer : migrated.consumers) {
		for (auto &share : consumer.shares) {
			if (share.id.base() != UINT32_MAX || share.parent.base() != UINT32_MAX || !share.segment.empty()) return UtilityPowerError::InvalidReference;
			if (share.imported_link.base() != UINT32_MAX) continue;
			if (migrated.next_share >= MAX_UTILITY_POWER_SHARES) return UtilityPowerError::Limit;
			share.id = UtilityShareID{migrated.next_share++};
			if (!roots.emplace(std::pair{consumer.meter.consumer_id, share.producer}, share.id).second) return UtilityPowerError::InvalidReference;
		}
	}
	// Preserve every original quantity and incoming edge. Missing or nonlocal
	// legacy parents refuse instead of manufacturing a new source reservation.
	for (auto &consumer : migrated.consumers) {
		for (auto &share : consumer.shares) {
			if (share.imported_link.base() == UINT32_MAX) continue;
			if (share.imported_link.base() >= migrated.links.size() || migrated.next_share >= MAX_UTILITY_POWER_SHARES) return UtilityPowerError::InvalidReference;
			const auto &link = migrated.links[share.imported_link.base()];
			const auto parent = roots.find(std::pair{link.source, share.producer});
			if (link.kind != UtilityLinkKind::ElectricalPortal || parent == roots.end()) return UtilityPowerError::InvalidTopology;
			share.id = UtilityShareID{migrated.next_share++}; share.parent = parent->second; share.segment = {link.id};
		}
	}
	const auto result = ValidateUtilityPowerLineage(migrated);
	if (result == UtilityPowerError::None) {
		// Native older rows used producer/final-edge order. Parent identities
		// remain fixed while current presentation adopts exact incoming rights.
		for (auto &consumer : migrated.consumers) {
			/** @param left First retained incoming right. @param right Second retained incoming right. @return Canonical source/parent/segment order without reissuing IDs. */
			std::ranges::sort(consumer.shares, [](const UtilityPowerShare &left, const UtilityPowerShare &right) {
				return std::tie(left.producer, left.parent, left.segment) < std::tie(right.producer, right.parent, right.segment);
			});
		}
		state = std::move(migrated);
	}
	return result;
}
