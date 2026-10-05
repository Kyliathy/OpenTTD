/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_commission.cpp Commissioning purchase of a boring machine at a Sealed rail portal: the one transaction
 * that acquires the consist, reserves its job, binds the pair and publishes the project, and the command that plans a
 * requested floor for the actual consist from the portal's door before buying it.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 2 (tasks 2.1, 2.3, 2.4)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 8 (controller handoff)
 */

#include "stdafx.h"
#include "tbm.h"
#include "tbm_cmd.h"
#include "tbm_exchange.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "company_func.h"
#include "train.h"
#include "map_func.h"
#include "table/strings.h"

#include "safeguards.h"

/** Longest side of a commissioning floor rectangle, in tiles; the planner bounds the job itself. */
static constexpr uint MAX_TBM_COMMISSION_AREA_SIDE = 64;

TBMCommissionResult CommissionTBMProject(const LayerPortalCommissionRequest &request, bool execute)
{
	TBMCommissionResult result;
	/**
	 * Finish the transaction.
	 * @param error Stable refusal, or None on success.
	 * @return The result carrying it.
	 */
	const auto finish = [&result](TBMAcquisitionError error) {
		result.error = error;
		return result;
	};

	/* Pure preparation: portal proof, definitions, route replay, pass and reservation quotes, pool, unit, funds. */
	const TBMAcquisitionPlan plan = PrepareTBMAcquisition(request, {request.earth_wagons, request.rock_wagons});
	result.portal_error = plan.portal_error;
	result.geology_error = plan.geology_error;
	result.failure_tile = plan.failure_tile;
	if (plan.error != TBMAcquisitionError::None) return finish(plan.error);

	/* Origin: real receivers for both spoils at the Surface origin, before any wagon exists (Plan 5 Group 4.2). */
	TBMProject probe;
	probe.company = request.proposal.company;
	probe.origin = request.proposal.origin;
	if (CheckTBMOriginAdmission(probe) != TBMExchangeError::None) return finish(TBMAcquisitionError::NoReceiver);

	/* Capacity: the project store and the pair binding must both accept the new identities. */
	if (CaptureTBMProjects().size() >= MAX_TBM_PROJECTS || GetNextTBMProjectID().base() >= UINT32_MAX - 1) return finish(TBMAcquisitionError::ProjectLimit);
	const auto binding = BindLayerPortalWorkCommissioning(plan.proof.pair_key, plan.proof.pair_revision, plan.prospective_job, VehicleID::Invalid(), request.proposal.origin, false);
	if (binding != LayerPortalWorkError::None) {
		result.portal_error = binding;
		return finish(binding == LayerPortalWorkError::Limit ? TBMAcquisitionError::ProjectLimit : TBMAcquisitionError::Portal);
	}
	result.cost = plan.machinery_cost + plan.wagon_cost;
	if (!execute) return finish(TBMAcquisitionError::None);

	/* Acquisition: the owner deletes the parts and restores the random seeds unless they are released below. */
	TBMAcquisition held;
	const auto acquired = AcquireTBMConsist(request, plan, held);
	if (acquired != TBMAcquisitionError::None) return finish(acquired);
	const auto job = ReserveExcavationJob(plan.job, true);
	if (job.error != ExcavationError::None) {
		result.geology_error = job.error;
		result.failure_tile = job.failure_tile;
		return finish(TBMAcquisitionError::Geology);
	}
	/** Undo the reservation if a later step refuses; its claims return to rock and nothing was cut. */
	const auto release_job = [&job]() {
		[[maybe_unused]] const auto cancelled = CancelExcavationJob(job.id);
		[[maybe_unused]] const auto retired = RetireExcavationJob(job.id);
	};

	/* Publication: the pair, then the project; the estimates above make both refusals unreachable. */
	const VehicleID cutter = held.GetFront()->index;
	const auto bound = BindLayerPortalWorkCommissioning(plan.proof.pair_key, plan.proof.pair_revision, job.id, cutter, request.proposal.origin, true);
	if (bound != LayerPortalWorkError::None) {
		release_job();
		result.portal_error = bound;
		return finish(TBMAcquisitionError::Portal);
	}
	TBMProject candidate = held.GetProject();
	candidate.job = job.id;
	if (PublishTBMProject(candidate, false) != TBMProjectError::None) {
		ReleaseLayerPortalWorkBinding(plan.proof.pair_key);
		release_job();
		return finish(TBMAcquisitionError::ProjectLimit);
	}
	TBMProject project = held.Release();
	project.job = job.id;
	[[maybe_unused]] const auto published = PublishTBMProject(project, true);
	assert(published == TBMProjectError::None);
	result.project = project.id;
	result.job = job.id;
	result.cutter = cutter;
	return finish(TBMAcquisitionError::None);
}

/**
 * Build the commissioning request for a rectangle of floor: the canonical planner runs for the actual consist from
 * the portal's door, with the one-cell working ring around the floor and the housing accepted as working space.
 * @param portal Upper rail end of the pair.
 * @param model_id Cutter model.
 * @param earth_wagons Earth hoppers.
 * @param rock_wagons Rock hoppers.
 * @param origin Surface origin station.
 * @param area_start One corner of the floor, on the rock layer.
 * @param area_end The opposite corner.
 * @param accept_partial Whether a plan covering only part of the floor is accepted.
 * @param[out] request The request, filled on success.
 * @return Nothing on success, or the refusal message.
 */
static std::optional<StringID> PlanTBMCommission(TileIndex portal, uint16_t model_id, uint8_t earth_wagons, uint8_t rock_wagons, StationID origin, TileIndex area_start, TileIndex area_end, bool accept_partial, LayerPortalCommissionRequest &request)
{
	if (!GetLayerPortalPairKey(portal)) return STR_ERROR_TBM_COMMISSION_PORTAL;
	LayerPortalCommissionRequest base;
	base.portal = portal;
	base.proposal.company = _current_company;
	base.proposal.origin = origin;
	base.proposal.model_id = model_id;
	base.proposal.commissioning_anchor = GetLayerPortalPairTile(portal);
	base.proposal.pass_terms.emplace();
	base.earth_wagons = earth_wagons;
	base.rock_wagons = rock_wagons;
	const auto proof = IssueLayerPortalBootstrap(base);
	// The portal preflight reports a date lock for both a Deep pair and a cutter not yet introduced; only the first is Deep.
	if (proof.error == LayerPortalWorkError::LockedDeep) return CheckLayerPortalAvailability(portal) == LayerPortalWorkError::LockedDeep ? STR_ERROR_EXCAVATION_DEEP_LOCKED : STR_ERROR_TBM_COMMISSION_MODEL;
	if (proof.error != LayerPortalWorkError::None) return STR_ERROR_TBM_COMMISSION_PORTAL;
	const auto dimensions = GetTBMConsistDimensions(model_id, {earth_wagons, rock_wagons});
	if (dimensions.error == TBMAcquisitionError::UnavailableModel) return STR_ERROR_TBM_COMMISSION_MODEL;
	if (dimensions.error != TBMAcquisitionError::None) return STR_ERROR_TBM_COMMISSION_CONSIST;

	/* The floor lies on the housing's own rock layer, as a bounded rectangle; the housing itself is never floor. */
	const MapLayer layer = GetTileLayer(proof.destination);
	if (!IsValidTile(area_start) || !IsValidTile(area_end) || GetTileLayer(area_start) != layer || GetTileLayer(area_end) != layer) return STR_ERROR_TBM_COMMISSION_AREA;
	const uint x0 = std::min(TileX(area_start), TileX(area_end)), x1 = std::max(TileX(area_start), TileX(area_end));
	const uint y0 = std::min(TilePlaneY(area_start), TilePlaneY(area_end)), y1 = std::max(TilePlaneY(area_start), TilePlaneY(area_end));
	if (x1 - x0 >= MAX_TBM_COMMISSION_AREA_SIDE || y1 - y0 >= MAX_TBM_COMMISSION_AREA_SIDE) return STR_ERROR_TBM_COMMISSION_AREA;
	std::vector<TileIndex> floor;
	for (uint y = y0; y <= y1; ++y) {
		for (uint x = x0; x <= x1; ++x) {
			const TileIndex tile = TileXYL(x, y, layer);
			if (tile != proof.destination) floor.push_back(tile);
		}
	}
	if (floor.empty()) return STR_ERROR_TBM_COMMISSION_AREA;
	std::set<TileIndex> extra;
	std::vector<TileIndex> centres = floor;
	centres.push_back(proof.destination);
	// The 3x3 cutter sweeps one cell beyond every floor cell and the housing it starts from; only that ring is offered.
	for (TileIndex centre : centres) {
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				const TileIndex cell = TileAddWrap(centre, dx, dy);
				if (IsValidTile(cell) && GetTileLayer(cell) == layer && !std::ranges::binary_search(floor, cell)) extra.insert(cell);
			}
		}
	}
	const auto requested = NormalizeExcavationOutline(layer, floor);
	if (requested.error != ExcavationError::None) return STR_ERROR_TBM_COMMISSION_AREA;
	const std::vector<TileIndex> accepted(extra.begin(), extra.end());

	/* The canonical planner, pass and reservation quotes for this consist, as the area preview shows them. */
	TBMAreaRequest area;
	area.outline = AuthorizeExcavationOutline(requested, accepted, model_id);
	if (area.outline.error != ExcavationError::None) return STR_ERROR_TBM_COMMISSION_AREA;
	area.start = {proof.destination, proof.exit_heading, std::vector<TileIndex>(dimensions.planning_points, proof.destination)};
	area.consist = {earth_wagons, rock_wagons};
	area.model_id = model_id;
	area.company = _current_company;
	area.origin = origin;
	const auto preview = PreviewTBMArea(area);
	if (preview.consist_error != TBMAcquisitionError::None) return STR_ERROR_TBM_COMMISSION_CONSIST;
	if (preview.error != ExcavationError::None) return STR_ERROR_TBM_COMMISSION_ROUTE;
	if (preview.partial && !accept_partial) return STR_ERROR_TBM_COMMISSION_PARTIAL;
	request = base;
	request.proposal = preview.proposal;
	return std::nullopt;
}

/**
 * @param result Refused commissioning.
 * @return The player's message for it.
 */
static StringID GetTBMCommissionErrorMessage(const TBMCommissionResult &result)
{
	switch (result.error) {
		case TBMAcquisitionError::Portal:
			if (result.portal_error != LayerPortalWorkError::LockedDeep) return STR_ERROR_TBM_COMMISSION_PORTAL;
			// A date lock is the Deep gate only when the pair itself is locked; otherwise the cutter is not introduced yet.
			return IsValidTile(result.failure_tile) && CheckLayerPortalAvailability(result.failure_tile) == LayerPortalWorkError::LockedDeep ? STR_ERROR_EXCAVATION_DEEP_LOCKED : STR_ERROR_TBM_COMMISSION_MODEL;
		case TBMAcquisitionError::Geology: return STR_ERROR_TBM_COMMISSION_ROUTE;
		case TBMAcquisitionError::UnavailableModel: return STR_ERROR_TBM_COMMISSION_MODEL;
		case TBMAcquisitionError::InvalidConsist: return STR_ERROR_TBM_COMMISSION_CONSIST;
		case TBMAcquisitionError::VehicleLimit:
		case TBMAcquisitionError::UnitLimit: return STR_ERROR_TOO_MANY_VEHICLES_IN_GAME;
		case TBMAcquisitionError::InsufficientFunds: return STR_ERROR_TBM_COMMISSION_FUNDS;
		case TBMAcquisitionError::NoReceiver: return STR_ERROR_TBM_COMMISSION_RECEIVER;
		case TBMAcquisitionError::ProjectLimit: return STR_ERROR_TBM_COMMISSION_LIMIT;
		default: return STR_ERROR_TBM_COMMISSION_FAILED; // Stale or a rolled-back assembly: nothing was bought.
	}
}

CommandCost CmdCommissionTBM(DoCommandFlags flags, TileIndex portal, uint16_t model_id, uint8_t earth_wagons, uint8_t rock_wagons, StationID origin, TileIndex area_start, TileIndex area_end, bool accept_partial)
{
	LayerPortalCommissionRequest request;
	if (const auto refusal = PlanTBMCommission(portal, model_id, earth_wagons, rock_wagons, origin, area_start, area_end, accept_partial, request)) return CommandCost(*refusal);
	const auto commissioned = CommissionTBMProject(request, flags.Test(DoCommandFlag::Execute));
	if (commissioned.error != TBMAcquisitionError::None) return CommandCost(GetTBMCommissionErrorMessage(commissioned));
	return CommandCost(ExpensesType::NewVehicles, commissioned.cost);
}
