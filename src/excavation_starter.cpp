/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file excavation_starter.cpp Solid commissioning start for geology: the shared 3x3 cutter envelope, the
 * issuer resolution through the portal owner and the first-segment rule. The pocket stays restricted
 * occupancy; only the first paid cut opens it.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 14B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-f-solid-starter-contract.md
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "stdafx.h"
#include "excavation.h"
#include "layer_portal_cmd.h"
#include "tile_map.h"

#include "safeguards.h"

/**
 * Build the 3x3 envelope with bounded, non-wrapping addressing.
 * @param centre Explicit world tile.
 * @return Nine ascending cells on the centre's plane, or empty.
 */
std::vector<TileIndex> GetExcavationCutterEnvelope(TileIndex centre)
{
	if (!IsValidTile(centre)) return {};
	std::vector<TileIndex> tiles;
	tiles.reserve(9);
	// Row-major offsets yield ascending tile order for binary search; one wrapped or off-plane cell voids the
	// whole envelope, so a map-edge housing can never be granted a partial pocket.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			const auto cell = TileAddWrap(centre, x, y);
			if (!IsValidTile(cell) || GetTileLayer(cell) != GetTileLayer(centre)) return {};
			tiles.push_back(cell);
		}
	}
	return tiles;
}

/**
 * Translate a portal issuer refusal into the geological error the job reports.
 * @param error Portal owner refusal.
 * @return Lost pair -> MissingSupport, owner -> InvalidOwner, date -> LockedDeep, other phase/assignment -> StaleParticipant.
 */
static ExcavationError ToExcavationStarterError(LayerPortalWorkError error)
{
	switch (error) {
		case LayerPortalWorkError::None: return ExcavationError::None;
		case LayerPortalWorkError::InvalidPair: return ExcavationError::MissingSupport;
		case LayerPortalWorkError::InvalidOwner: return ExcavationError::InvalidOwner;
		case LayerPortalWorkError::LockedDeep: return ExcavationError::LockedDeep;
		default: return ExcavationError::StaleParticipant;
	}
}

/**
 * Resolve a solid start only while the anchor housing's own rock cell is unopened.
 * @param job Proposal or published job.
 * @return No start, current issuer with heading/pocket, or a stable refusal at the anchor.
 */
ExcavationStarterResult ResolveExcavationStarter(const ExcavationJob &job)
{
	ExcavationStarterResult result;
	const auto &geology = GetGeologyWorldState();
	if (geology.generator_version == 0 || !IsValidTile(job.commissioning_anchor)) return result;
	const auto anchor = geology.terrain.Get(job.commissioning_anchor);
	// An opened anchor (or a non-rock address, refused elsewhere) keeps the unchanged open-starter rules.
	if (!anchor || *anchor == ExcavationState::Excavated) return result;
	result.required = true;
	result.failure_tile = job.commissioning_anchor;
	result.pocket = GetExcavationCutterEnvelope(job.commissioning_anchor);
	if (result.pocket.empty()) {
		result.error = ExcavationError::MissingSupport;
		return result;
	}
	const auto issuer = GetLayerPortalStarterIssuer(job.commissioning_anchor, job.company, job.id);
	result.error = ToExcavationStarterError(issuer.error);
	if (result.error != ExcavationError::None) return result;
	result.exit_heading = issuer.exit_heading;
	result.failure_tile = INVALID_TILE;
	return result;
}

/**
 * Check route[0] against the issuer's door heading and pocket.
 * @param job Canonical route.
 * @param exit_heading Issuer door heading.
 * @param pocket Canonical starter envelope.
 * @return Stable refusal or success.
 */
ExcavationFootprintResult CheckExcavationStarterGeometry(const ExcavationJob &job, Direction exit_heading, std::span<const TileIndex> pocket)
{
	if (job.route.empty() || pocket.empty()) return {ExcavationError::InvalidRoute, job.commissioning_anchor};
	const auto &first = job.route.front();
	// The housing track is straight: the cutter can only leave through the door, never with a 45-degree start.
	if (first.heading != exit_heading) return {ExcavationError::IllegalTurn, first.cutter_position};
	// Every pocket cell must be swept so cut 0 opens and pays for all of the unopened starter rock.
	for (TileIndex cell : pocket) {
		if (!std::binary_search(first.swept_tiles.begin(), first.swept_tiles.end(), cell)) return {ExcavationError::InvalidRoute, cell};
	}
	// The straddling consist is supported by the housing and shaft only; support claimed outside the pocket would be unbacked rock.
	for (TileIndex cell : first.support_tiles) {
		if (!std::binary_search(pocket.begin(), pocket.end(), cell)) return {ExcavationError::MissingSupport, cell};
	}
	return {};
}
