/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_portal_bootstrap.cpp Portal-issued solid commissioning start: the bootstrap proof for the TBM
 * controller and the issuer query that geology consumes. Geometry and references only; nothing here
 * excavates, admits a vehicle, allocates a project or moves money.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 8B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-f-solid-starter-contract.md
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "stdafx.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "company_base.h"
#include "tile_map.h"

#include "safeguards.h"

/**
 * Check that a destination pocket is still free rock for this start, without reading private geology state.
 * @param pocket Destination 3x3 envelope from GetExcavationCutterEnvelope.
 * @param destination Issuing housing, the only structure the first cut may sweep.
 * @return None, or Conflict when another job, active cut or structure already occupies a pocket cell.
 */
static LayerPortalWorkError CheckBootstrapPocket(std::span<const TileIndex> pocket, TileIndex destination)
{
	const auto &geology = GetGeologyWorldState();
	// Every pocket cell will be swept by the first paid cut. A cell already claimed by another job or being
	// cut cannot be promised to this start, and no structure other than the issuing housing may be swept.
	for (TileIndex cell : pocket) {
		if (geology.job_claims.contains(cell)) return LayerPortalWorkError::Conflict;
		if (geology.terrain.Get(cell) == ExcavationState::Excavating) return LayerPortalWorkError::Conflict;
		if (cell != destination && GetTileType(cell) != TileType::Clear) return LayerPortalWorkError::Conflict;
	}
	return LayerPortalWorkError::None;
}

/**
 * Issue the proof from the existing pure commissioning preflight.
 * @param request Actual upper rail pair and its unallocated geological proposal.
 * @return Complete proof, or one carrying only the preflight refusal and pair key.
 */
LayerPortalBootstrapProof IssueLayerPortalBootstrap(const LayerPortalCommissionRequest &request)
{
	LayerPortalBootstrapProof proof;
	const auto preflight = ValidateLayerPortalCommissioning(request);
	proof.error = preflight.error;
	proof.pair_key = preflight.pair_key;
	if (preflight.error != LayerPortalWorkError::None) return proof;
	// The preflight proved an owned Sealed upper rail end with free shafts and an accessible 3x3 source floor.
	const auto destination = GetLayerPortalPairTile(request.portal);
	auto source_support = GetExcavationCutterEnvelope(request.portal);
	auto pocket = GetExcavationCutterEnvelope(destination);
	// Both envelopes must lie wholly on their own planes; an edge housing has no complete straddling floor or pocket.
	if (source_support.empty() || pocket.empty()) {
		proof.error = LayerPortalWorkError::InvalidPair;
		return proof;
	}
	proof.error = CheckBootstrapPocket(pocket, destination);
	if (proof.error != LayerPortalWorkError::None) return proof;
	proof.pair_revision = preflight.pair_revision;
	proof.company = request.proposal.company;
	proof.model_id = request.proposal.model_id;
	proof.origin = request.proposal.origin;
	proof.source = request.portal;
	proof.source_support = std::move(source_support);
	proof.destination = destination;
	proof.exit_heading = DiagDirToDir(GetLayerPortalDoorDirection(destination));
	proof.pocket = std::move(pocket);
	return proof;
}

/**
 * Re-issue against live state and require the earlier proof to be unchanged.
 * @param proof Earlier issued proof.
 * @param request Same pair and proposal.
 * @return None, the fresh refusal, or Conflict for any changed issued field.
 */
LayerPortalWorkError RevalidateLayerPortalBootstrap(const LayerPortalBootstrapProof &proof, const LayerPortalCommissionRequest &request)
{
	if (proof.error != LayerPortalWorkError::None) return proof.error;
	const auto fresh = IssueLayerPortalBootstrap(request);
	if (fresh.error != LayerPortalWorkError::None) return fresh.error;
	return fresh == proof ? LayerPortalWorkError::None : LayerPortalWorkError::Conflict;
}

/**
 * Resolve the pair that may issue a solid start for a geological job or proposal.
 * @param anchor Rock-side destination housing.
 * @param company Job owner.
 * @param job Invalid for an unpublished proposal, otherwise the published job.
 * @return Current issuer key and exit heading, or a stable refusal.
 */
LayerPortalStarterIssuer GetLayerPortalStarterIssuer(TileIndex anchor, CompanyID company, ExcavationJobID job)
{
	LayerPortalStarterIssuer result;
	const auto key = GetLayerPortalPairKey(anchor);
	// The anchor must be the rock-side destination of a real pair; the accessible source key never starts a cut.
	if (!key || *key == anchor) return result;
	const auto *record = GetLayerPortalWorkRecord(anchor);
	if (record == nullptr || GetLayerPortalTransportType(anchor) != TransportType::Rail) return result;
	result.pair_key = *key;
	// The job owner must be a real company that owns the issuing pair; a foreign proposal cannot borrow its pocket.
	if (!Company::IsValidID(company) || record->company != company) {
		result.error = LayerPortalWorkError::InvalidOwner;
		return result;
	}
	result.error = CheckLayerPortalAvailability(anchor);
	if (result.error != LayerPortalWorkError::None) return result;
	const bool unpublished = job == ExcavationJobID{UINT32_MAX};
	// Before publication only an empty Sealed pair may issue; after publication only the pair that is
	// Commissioning exactly this job. Any other phase or assignment means the start is stale.
	const bool sealed_quote = unpublished && record->state == LayerPortalWorkState::Sealed && record->project == ExcavationJobID{UINT32_MAX} && record->cutter == VehicleID::Invalid();
	const bool published = !unpublished && record->state == LayerPortalWorkState::Commissioning && record->project == job;
	if (!sealed_quote && !published) {
		result.error = LayerPortalWorkError::Conflict;
		return result;
	}
	result.exit_heading = DiagDirToDir(GetLayerPortalDoorDirection(anchor));
	result.error = LayerPortalWorkError::None;
	return result;
}
