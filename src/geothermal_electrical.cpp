/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_electrical.cpp Bounded deterministic paid-riser routing and metered export through the sole shared utility owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5
 */

#include "stdafx.h"
#include "geothermal_electrical.h"
#include "geothermal.h"
#include "geothermal_riser.h"
#include "layer_portal_cmd.h"
#include "company_base.h"
#include "company_func.h"
#include "tile_map.h"
#include "openttd.h"
#include <set>
#include "safeguards.h"

/** Transient search predecessor; references physical paths only, with no cached or private energy balance. */
struct ElectricalPathCandidate {
	UtilityConsumerID consumer{UINT32_MAX}; ///< Exact account at this arriving housing.
	TileIndex port = INVALID_TILE; ///< Physical arrival, distinct from a shared same-layer account identity.
	size_t previous = SIZE_MAX; ///< Earlier candidate, bounding reconstruction and cycle checks.
	GeothermalElectricalLeg leg; ///< Paid transition and explicit local inlet from the predecessor.
	uint64_t capacity_w = MAX_UTILITY_RATE_W; ///< Minimum physical rate along this candidate's original path.
};

/** @param link Candidate shared edge. @param owner Actual source controller. @return Whether the exact original paid machinery/pair is currently operational; older generic roles supply no paid-route fallback. */
static bool IsElectricalPathRiser(const UtilityPowerLink &link, CompanyID owner)
{
	// Physical loss, incomplete work or a different native portal incarnation
	// cannot be bypassed by matching numerical participant references.
	if (!link.active || link.kind != UtilityLinkKind::ElectricalRiser || !IsGeothermalRiserOperational(link.riser)) return false;
	const auto *state = GetGeothermalRiserState(link.riser);
	const auto *from = GetUtilityPowerConsumer(link.source), *to = GetUtilityPowerConsumer(link.destination);
	return state != nullptr && from != nullptr && to != nullptr && from->active && to->active && from->owner == owner && to->owner == owner &&
		from->meter.domain == UtilityEnergyDomain::Electricity && to->meter.domain == UtilityEnergyDomain::Electricity &&
		state->site.owner == owner && state->site.portal == link.source_endpoint && state->destination == link.destination_endpoint &&
		state->portal_revision == link.portal_revision && from->meter.layer == GetTileLayer(link.source_endpoint) && to->meter.layer == GetTileLayer(link.destination_endpoint);
}

/** @param candidates Bounded predecessor graph. @param current Current physical arrival. @param destination Proposed next reservation. @return Whether the candidate would revisit an earlier account and circulate its imported energy. */
static bool RepeatsElectricalAccount(const std::vector<ElectricalPathCandidate> &candidates, size_t current, UtilityConsumerID destination)
{
	// Walk the already bounded predecessor chain, rather than keeping another
	// mutable account graph or accepting an electrical circulation as a route.
	for (size_t index = current; index != SIZE_MAX; index = candidates[index].previous) {
		if (candidates[index].consumer == destination) return true;
	}
	return false;
}

/** @copydoc FindGeothermalElectricalPath */
std::optional<GeothermalElectricalPath> FindGeothermalElectricalPath(UtilityConsumerID source, UtilityConsumerID destination,
	TileIndex source_port, uint32_t economy_ticks, std::span<const UtilityLinkID> excluded)
{
	const auto *from = GetUtilityPowerConsumer(source), *to = GetUtilityPowerConsumer(destination);
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	// The actual technology, controller and electrical planes govern routing;
	// a view, shared ID or arbitrary interval quote cannot create availability.
	if (_game_mode != GameMode::Normal || profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy ||
		!CheckDeepLayerAvailability(profile, TimerGameCalendar::date) || from == nullptr || to == nullptr || !from->active || !to->active ||
		!Company::IsValidID(from->owner) || from->owner != to->owner || source == destination ||
		from->meter.domain != UtilityEnergyDomain::Electricity || to->meter.domain != UtilityEnergyDomain::Electricity ||
		!IsValidTile(source_port) || GetTileLayer(source_port) != from->meter.layer || economy_ticks == 0 || economy_ticks > MAX_UTILITY_INTERVAL_TICKS ||
		excluded.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return std::nullopt;
	const std::set<UtilityLinkID> unavailable(excluded.begin(), excluded.end());
	std::vector<ElectricalPathCandidate> candidates{{source, source_port}};
	size_t path_references = 0;
	// Breadth-first expansion selects fewest paid transitions, then shared ID
	// order. Each arrival retains its actual port: equal account IDs at remote
	// housings never substitute for an installed relay cable. Candidate and
	// cumulative cable-reference caps bound dense alternatives without caches.
	for (size_t current = 0; current < candidates.size(); ++current) {
		const auto account = candidates[current].consumer;
		const auto port = candidates[current].port;
		for (const auto &link : GetUtilityPowerLinks()) {
			if (link.source != account || unavailable.contains(link.id) || !IsElectricalPathRiser(link, from->owner)) continue; // Only this physical account's available original paid directions can extend the route.
			if (RepeatsElectricalAccount(candidates, current, link.destination)) continue;
			const auto *state = GetGeothermalRiserState(link.riser);
			auto inlet = FindGeothermalUtilityPath(from->owner, UtilityEnergyDomain::Electricity, port, state->site.port, excluded);
			if (!inlet.has_value()) continue;
			if (candidates.size() >= MAX_UTILITY_POWER_CONNECTIONS || path_references + inlet->cells.size() > MAX_UTILITY_POWER_SHARES) return std::nullopt; // Refuse a whole over-budget quote before exposing a partial route or touching a ledger.
			path_references += inlet->cells.size();
			const auto rate = std::min({candidates[current].capacity_w, inlet->capacity_w, link.riser_capacity_w});
			candidates.push_back({link.destination, link.destination_endpoint, current, {link.id, std::move(*inlet)}, rate});
			if (link.destination != destination) continue;
			const auto integrated = IntegrateUtilityGeneration(rate, economy_ticks, 0);
			if (!integrated.has_value()) return std::nullopt;
			GeothermalElectricalPath result{GetUtilityLocalLinkRevision(), rate, integrated->energy_j, {}};
			// Reconstruct only the accepted original predecessor chain. Local
			// cable and paid identities retain their order for later settlement.
			for (size_t index = candidates.size() - 1; candidates[index].previous != SIZE_MAX; index = candidates[index].previous) {
				result.legs.push_back(std::move(candidates[index].leg));
			}
			std::ranges::reverse(result.legs);
			return result;
		}
	}
	return std::nullopt;
}

/** @copydoc PrepareGeothermalPowerExport */
GeothermalPowerExportProposal PrepareGeothermalPowerExport(const GeothermalPowerExportRequest &request)
{
	GeothermalPowerExportProposal refused;
	refused.request.grant = request.grant; refused.request.destination = request.destination;
	refused.request.source_port = request.source_port; refused.request.quantity_j = request.maximum_j;
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	// Only actual current native gameplay can dispatch paid export; editor or
	// preserved legacy policy cannot acquire physical electrical rights.
	if (_game_mode != GameMode::Normal || profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy) {
		refused.error = UtilityPowerError::UnsupportedProfile; return refused;
	}
	const auto *source = GetUtilityPowerConsumer(request.grant.consumer), *target = GetUtilityPowerConsumer(request.destination);
	if (source == nullptr || target == nullptr) return refused; // Both output participants must already exist; this feature never creates a receiving account.
	// Physical discovery checks the same actual asking controller as final
	// settlement. Retired/foreign consumers cannot quote a usable export.
	if (!source->active || !target->active || source->owner != _current_company || source->owner != target->owner) {
		refused.error = UtilityPowerError::InvalidOwner; return refused;
	}
	// Heat stays on its native Deep network. Self-dispatch and a source port
	// on another plane cannot masquerade as an installed electrical inlet.
	if (source->meter.domain != UtilityEnergyDomain::Electricity || target->meter.domain != UtilityEnergyDomain::Electricity ||
		request.grant.consumer == request.destination || !IsValidTile(request.source_port) || GetTileLayer(request.source_port) != source->meter.layer) {
		refused.error = UtilityPowerError::InvalidTopology; return refused;
	}
	if (request.maximum_j == 0 || request.maximum_j > MAX_UTILITY_ACCOUNT_ENERGY_J) { refused.error = UtilityPowerError::InvalidQuantity; return refused; } // A bounded positive whole action is required before physical search or shared preparation.
	if (request.excluded.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) { refused.error = UtilityPowerError::Limit; return refused; }
	if (!IsUtilityPowerAllocated()) { refused.error = UtilityPowerError::AllocationRequired; return refused; }
	if (request.grant.interval != GetUtilityPowerInterval()) { refused.error = UtilityPowerError::InvalidInterval; return refused; }
	// One tick is a geometry-discovery argument only. Its quoted joules are
	// deliberately discarded: original producers' actual saved durations and
	// shared aggregate residuals exclusively decide the admitted quantity.
	const auto path = FindGeothermalElectricalPath(request.grant.consumer, request.destination, request.source_port, 1, request.excluded);
	if (!path.has_value()) { refused.error = UtilityPowerError::InvalidTopology; return refused; }
	auto transfer = refused.request; transfer.legs.reserve(path->legs.size());
	// Preserve every original paid edge and ordered inlet before asking the
	// shared owner for a complete offer; intermediate accounts acquire no grant.
	for (const auto &leg : path->legs) transfer.legs.push_back({leg.riser, leg.inlet.cells});
	const auto offer = GetUtilityPathTransferOffer(transfer);
	if (offer.error != UtilityPowerError::None) { refused.error = offer.error; return refused; }
	transfer.quantity_j = offer.quantity_j;
	return PrepareUtilityPathTransfer(transfer);
}

/** @copydoc ExportGeothermalPower */
UtilityMutationReceipt ExportGeothermalPower(const GeothermalPowerExportProposal &proposal)
{
	// Whole shared revalidation/publication owns original source slices,
	// lineage, every paid flow and the one receipt. Serial per-leg transfers or
	// a feature-local balance would permit partial export or duplicate supply.
	return CommitUtilityPathTransfer(proposal);
}
