/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_return_target.cpp Original mining route domains before physical receiver selection.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group19.
 */
#include "stdafx.h"
#include "space_return_target.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_mining_tuning.h"
#include "space_approach.h"
#include "space_structure.h"
#include "space_structure_cmd.h"
#include "orbital_service.h"
#include "launch_center.h"
#include "launch_center_cmd.h"
#include "space_pod.h"
#include "station_base.h"
#include "company_base.h"
#include "company_func.h"
#include "map_func.h"
#include "core/backup_type.hpp"
#include <tuple>
#include "safeguards.h"

/** @copydoc GetSpaceReturnRoute */
std::optional<SpaceReturnRoute> GetSpaceReturnRoute(const SpaceVehicle &craft)
{
	return GetSpaceReturnRoute(GetSpaceVehicleModel(craft), craft.space_orders, craft.manifest && !craft.manifest->pods.empty(), craft.construction.home);
}

/** @copydoc GetSpaceReturnRoute(uint16_t, const SpaceOrderQueue &, bool, LaunchCenterRef) */
std::optional<SpaceReturnRoute> GetSpaceReturnRoute(uint16_t model, const SpaceOrderQueue &queue, bool has_pods, LaunchCenterRef home)
{
	// Return continuation belongs only to the original supported miner and a
	// bounded current queue; it cannot take over another action's native cursor.
	if (GetSpaceMiningTuning(model) == nullptr || queue.entries.size() > MAX_SPACE_ORDERS || queue.cursor > queue.entries.size()) return std::nullopt;
	SpaceReturnRoute result;
	result.handling_index = static_cast<uint8_t>(queue.entries.size());
	if (queue.cursor < queue.entries.size()) {
		const auto &mine = queue.entries[queue.cursor];
		if (mine.action != SpaceOrderAction::Mine || mine.mining_policy >= SpaceMiningPolicy::End) return std::nullopt;
		result.nearest = mine.mining_policy != SpaceMiningPolicy::LegacyReturn;
		result.visits_begin = queue.cursor + 1;
		// Visits remain explicit original instructions. Another work action is a
		// boundary, not permission to skip ahead to a later unrelated receiver.
		for (size_t index = result.visits_begin; index < queue.entries.size(); ++index) {
			const auto &order = queue.entries[index];
			if (order.action == SpaceOrderAction::Visit) continue;
			if ((order.action != SpaceOrderAction::Unload && order.action != SpaceOrderAction::ReturnService) ||
				(order.target.kind != SpaceOrderTargetKind::LaunchCenter && order.target.kind != SpaceOrderTargetKind::Structure)) return std::nullopt;
			result.domain = order.target.kind; result.specified = order.target;
			result.handling_index = static_cast<uint8_t>(index); result.declared_action = order.action;
			return result;
		}
	}
	// Original small miners use their Surface recovery domain. A factory-only
	// heavy needs an explicit module route or actual carried shells before an
	// implicit Surface return is meaningful; this grants no shell or quota.
	if (model != SPACE_PROBE_MODEL && !has_pods) return std::nullopt;
	if (home.station != StationID::Invalid() && home.generation != 0) result.specified = SpaceOrderTarget{SpaceOrderTargetKind::LaunchCenter, home.station.base(), home.generation};
	// A nearest policy can seek another owned real center after home loss. A
	// legacy coordinate-only return keeps its existing controller, not a newly
	// invented facility or automatic replacement destination.
	if (!result.nearest && !result.specified) return std::nullopt;
	return result;
}

/** @copydoc GetSpaceReturnCandidateAdmission */
std::optional<SpaceReturnCandidate> GetSpaceReturnCandidateAdmission(const SpaceVehicle &craft, const SpaceOrderTarget &target, SpaceReturnPurpose purpose)
{
	const auto route = GetSpaceReturnRoute(craft);
	// The original instruction limits the domain; a matching label or closer
	// foreign site cannot invent a new return route or revive a stale exact target.
	if (!route || purpose >= SpaceReturnPurpose::End || target.kind != route->domain || target.generation == 0 ||
		(!route->nearest && route->specified != target) || !Company::IsValidID(craft.owner) ||
		craft.construction.placement != SpacePlacement::Orbital || craft.vehstatus.Test(VehState::Crashed) ||
		HasOrbitalServiceAssignment(craft) || (craft.manifest && craft.manifest->service_ticks != 0)) return std::nullopt;
	const auto held = craft.cargo.TotalCount();
	const auto mineral = craft.mission.continuation.mineral;
	// Real held freight must retain its pinned mineral. Empty navigation may
	// return without a fake positive cargo offer, pod expenditure or feeder credit.
	if (held != 0 && (!IsValidCargoType(craft.cargo_type) || (!mineral.Empty() && CargoSpec::Get(craft.cargo_type)->label != mineral))) return std::nullopt;
	// Automatic service is genuinely due, not a request to buy another bill to
	// solve missing pods or pending recovery. Existing paused work keeps its owner.
	if (purpose == SpaceReturnPurpose::Service && (!craft.manifest || craft.manifest->returns_remaining != 0 || craft.manifest->pending_returns != 0)) return std::nullopt;
	// A no-bill service visit is valid only while the original allowance really
	// remains usable; pending descents still own their existing return claims.
	if (purpose == SpaceReturnPurpose::ServiceVisit && (!craft.manifest || craft.manifest->returns_remaining == 0 || craft.manifest->pending_returns != 0)) return std::nullopt;
	SpaceReturnCandidate result; result.target = target; result.purpose = purpose;
	const auto ref = GetSpaceCraftReference(craft);
	if (target.kind == SpaceOrderTargetKind::LaunchCenter) {
		if (target.id >= StationID::End().base()) return std::nullopt;
		const LaunchCenterRef center_ref{StationID{static_cast<uint16_t>(target.id)}, target.generation};
		const auto *center = ResolveLaunchCenter(center_ref);
		if (center == nullptr || center->owner != craft.owner) return std::nullopt;
		const auto anchor = GetLaunchCenterPlaneReference(center->launch_center->tile); if (!anchor) return std::nullopt;
		result.anchor = *anchor;
		if (purpose == SpaceReturnPurpose::Service) return CanNavigateSpaceOnboardService(ref) ? std::optional{result} : std::nullopt;
		if (purpose == SpaceReturnPurpose::ServiceVisit) return result; // The original ReturnService may finish without buying service when allowance remains.
		if (held == 0) return result; // Real empty navigation grants no cargo transaction or shell rights.
		if (!craft.manifest) return std::nullopt;
		result.quantity = static_cast<uint16_t>(std::min<uint>(40, held));
		std::optional<SpacePodRef> chosen;
		// Every offered shell is an actual carried native asset. Stable identity
		// chooses among independently admitted real batches without reordering or
		// reserving the manifest; later release repeats the complete preflight.
		for (const auto pod : craft.manifest->pods) {
			if (!CanNavigateOrbitalCargoRelease(ref, center_ref, pod, result.quantity)) continue;
			if (!chosen || std::pair{pod.pod.base(), pod.generation} < std::pair{chosen->pod.base(), chosen->generation}) chosen = pod;
		}
		if (!chosen) return std::nullopt;
		result.pod = *chosen; return result;
	}
	if (target.kind != SpaceOrderTargetKind::Structure || target.id >= SpaceStructureID::End().base()) return std::nullopt;
	const SpaceStructureRef site_ref{SpaceStructureID{static_cast<uint16_t>(target.id)}, target.generation};
	const auto *site = ResolveSpaceStructure(site_ref);
	if (site == nullptr || site->state.owner != craft.owner || !IsOrbitalModuleOperational(*site)) return std::nullopt;
	result.anchor = GetSpaceStructureApproach(site->state);
	if (purpose == SpaceReturnPurpose::Service) return CanNavigateOrbitalService(ref, site_ref) ? std::optional{result} : std::nullopt;
	if (purpose == SpaceReturnPurpose::ServiceVisit) {
		// A reached ReturnService with positive allowance visits a real service
		// function; it cannot silently become an unload into arbitrary stock.
		return GetOrbitalModuleServiceCapacity(site->state.kind) != 0 && CanAcquireSpaceApproach(craft, target) ? std::optional{result} : std::nullopt;
	}
	if (held == 0) {
		const auto cargo = mineral.Empty() ? craft.cargo_type : GetCargoTypeByLabel(mineral);
		// An empty return still identifies a real compatible receiving function,
		// but cannot borrow a fabricated one-unit quote or spend a return allowance.
		if (!IsValidCargoType(cargo) || GetOrbitalModuleCargoCapacity(*site, cargo) == 0 || !CanAcquireSpaceApproach(craft, target)) return std::nullopt;
		return result;
	}
	if (held > UINT16_MAX || !CanNavigateOrbitalFreightUnload(ref, site_ref, static_cast<uint16_t>(held))) return std::nullopt;
	result.quantity = static_cast<uint16_t>(held); return result;
}

/** @copydoc FindNearestEligibleSpaceReturn */
std::optional<SpaceReturnCandidate> FindNearestEligibleSpaceReturn(const SpaceVehicle &craft, SpaceReturnPurpose purpose)
{
	const auto route = GetSpaceReturnRoute(craft); const auto bounds = GetSpacePlaneBounds(Map::SizeX(), Map::SizeY());
	if (!route || !bounds || !bounds->Contains(craft.position) || purpose >= SpaceReturnPurpose::End) return std::nullopt;
	if (!route->nearest) return route->specified ? GetSpaceReturnCandidateAdmission(craft, *route->specified, purpose) : std::nullopt;
	std::optional<SpaceReturnCandidate> best;
	/** @param candidate Original admitted destination. @return Stable exact comparison key, independent of pool iteration or an unreserved Dock slot. */
	const auto key = [&](const SpaceReturnCandidate &candidate) {
		return std::tuple{SpaceDistanceSquared(craft.position, candidate.anchor), to_underlying(candidate.target.kind), candidate.target.id, candidate.target.generation};
	};
	/** @param target Exact original facility in the permitted domain. Inspect actual native admission and retain only a strictly better deterministic key. */
	const auto consider = [&](SpaceOrderTarget target) {
		const auto candidate = GetSpaceReturnCandidateAdmission(craft, target, purpose);
		if (candidate && (!best || key(*candidate) < key(*best))) best = *candidate;
	};
	// Enumerate the whole original allowed owner family. No fixed shortlist or
	// copied stock account can hide a farther eligible receiver behind a full one.
	if (route->domain == SpaceOrderTargetKind::LaunchCenter) {
		for (const auto *center : Station::Iterate()) {
			if (!center->launch_center) continue;
			const auto ref = GetLaunchCenterReference(*center);
			consider({SpaceOrderTargetKind::LaunchCenter, ref.station.base(), ref.generation});
		}
	} else if (route->domain == SpaceOrderTargetKind::Structure) {
		for (const auto *site : SpaceStructure::Iterate()) consider({SpaceOrderTargetKind::Structure, site->index.base(), site->generation});
	}
	return best;
}

/** @copydoc ExecuteSpaceReturnArrival */
bool ExecuteSpaceReturnArrival(SpaceVehicle &craft, const SpaceOrderTarget &target, SpaceReturnPurpose purpose)
{
	const auto fresh = GetSpaceReturnCandidateAdmission(craft, target, purpose);
	if (!fresh) return false;
	AutoRestoreBackup actor(_current_company, craft.owner);
	if (target.kind == SpaceOrderTargetKind::LaunchCenter) {
		// Positive freight keeps its original inclusive drop circle and real pod
		// release. Empty return and onboard-service navigation reach the exact anchor.
		if (purpose == SpaceReturnPurpose::Cargo && fresh->quantity != 0) return CommitOrbitalCargoRelease(GetSpaceCraftReference(craft),
			{StationID{static_cast<uint16_t>(target.id)}, target.generation}, fresh->pod, fresh->quantity);
		if (craft.position != fresh->anchor) return false;
		return purpose != SpaceReturnPurpose::Service || CmdServiceLaunchProbe(DoCommandFlag::Execute, craft.index, craft.generation, false).Succeeded();
	}
	const SpaceStructureRef ref{SpaceStructureID{static_cast<uint16_t>(target.id)}, target.generation};
	if (purpose == SpaceReturnPurpose::ServiceVisit) {
		const auto *site = ResolveSpaceStructure(ref);
		// Completing the original service visit still needs its real physical
		// berth; it transfers no freight and cannot grant another allowance.
		return site != nullptr && GetSpaceStructureBerth(craft, *site).has_value();
	}
	if (purpose == SpaceReturnPurpose::Service) return CmdServiceOrbitalProbe(DoCommandFlag::Execute, ref.structure, ref.generation,
		craft.index, craft.generation, OrbitalServiceControl::Start).Succeeded();
	if (fresh->quantity == 0) {
		const auto *site = ResolveSpaceStructure(ref); if (site == nullptr) return false;
		// Empty arrival retains the original role's physical rule without issuing
		// a zero-quantity exchange or pretending a heavy hull owns another berth.
		return site->state.kind == SpaceBuildKind::Dock || GetSpaceVehicleModel(craft) == SPACE_HEAVY_MINER_MODEL ?
			GetSpaceStructureBerth(craft, *site).has_value() : craft.position == fresh->anchor;
	}
	return CmdMoveOrbitalFreight(DoCommandFlag::Execute, ref.structure, ref.generation, craft.index, craft.generation,
		craft.cargo_type, fresh->quantity, false).Succeeded();
}
