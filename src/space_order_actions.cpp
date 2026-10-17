/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_order_actions.cpp Connect reached typed instructions to existing freight and paid-service producers.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B
 * Navigation never substitutes for a packet exchange, actual service materials or normal shared work.
 */
#include "stdafx.h"
#include "space_order_actions.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_structure_cmd.h"
#include "launch_center_cmd.h"
#include "orbital_service.h"
#include "company_func.h"
#include "core/backup_type.hpp"
#include "safeguards.h"

/** @copydoc IsSpaceReturnServiceWait */
bool IsSpaceReturnServiceWait(const SpaceOrderQueue &queue)
{
	// Only the reached current action may coexist with service; future entries, scan work and an exhausted travel deadline provide no authority.
	return queue.started && queue.cursor < queue.entries.size() && queue.entries[queue.cursor].action == SpaceOrderAction::ReturnService &&
		queue.timer_ticks != 0 && queue.work_ticks == 0 && queue.blocked != SpaceOrderBlock::Timeout;
}

/** @copydoc IsSpaceOnboardServiceOrder */
bool IsSpaceOnboardServiceOrder(const SpaceVehicle &craft)
{
	if (!IsSpaceReturnServiceWait(craft.space_orders)) return false;
	const auto &target = craft.space_orders.entries[craft.space_orders.cursor].target;
	SpacePoint point;
	// Onboard parts can service only this explicit reached owned center instruction; a module request cannot borrow that independent service owner.
	if (target.kind != SpaceOrderTargetKind::LaunchCenter || craft.construction.placement != SpacePlacement::Orbital || craft.mission.phase != SpaceMissionPhase::Holding) return false;
	const auto resolution = ResolveSpaceNavigationTarget(craft, target, point);
	if (resolution == SpaceOrderBlock::None) return craft.position == point;
	// Center retirement cannot erase an already owned onboard service timer or its parts. Only explicitly marked lost history may retain that existing producer; it cannot start a new timer at an absent/foreign station.
	return resolution == SpaceOrderBlock::TargetRemoved && craft.space_orders.blocked == SpaceOrderBlock::TargetRemoved && craft.manifest != nullptr &&
		craft.manifest->service_location == SpaceServiceLocation::Onboard && craft.manifest->service_ticks != 0;
}

/** @copydoc ForgetSpaceServiceOrderCenter */
void ForgetSpaceServiceOrderCenter(const LaunchCenterRef &center)
{
	// Only an existing actual onboard producer retains lost-target history. Future return instructions and idle craft cannot acquire service or allowance from a station's removal.
	for (auto *craft : SpaceVehicle::Iterate()) {
		if (!craft->manifest || craft->manifest->service_location != SpaceServiceLocation::Onboard || craft->manifest->service_ticks == 0 || !IsSpaceReturnServiceWait(craft->space_orders)) continue;
		const auto &target = craft->space_orders.entries[craft->space_orders.cursor].target;
		if (target.kind == SpaceOrderTargetKind::LaunchCenter && target.id == center.station.base() && target.generation == center.generation) craft->space_orders.blocked = SpaceOrderBlock::TargetRemoved;
	}
}

/** @copydoc ExecuteSpaceOrderUnload */
bool ExecuteSpaceOrderUnload(SpaceVehicle &craft)
{
	if (craft.cargo.TotalCount() == 0) return true;
	const auto &target = craft.space_orders.entries[craft.space_orders.cursor].target;
	AutoRestoreBackup actor(_current_company, craft.owner);
	// The original exchange command preflights the whole destination capacity, exact module generation/berth and every physical packet before moving any freight. A full or unavailable module retains the old hold.
	const auto result = CmdMoveOrbitalFreight(DoCommandFlag::Execute, SpaceStructureID{static_cast<uint16_t>(target.id)}, target.generation,
		craft.index, craft.generation, craft.cargo_type, static_cast<uint16_t>(craft.cargo.TotalCount()), false);
	if (result.Failed()) craft.space_orders.blocked = SpaceOrderBlock::ProducerUnavailable;
	return result.Succeeded();
}

/** @copydoc ExecuteSpaceOrderService */
bool ExecuteSpaceOrderService(SpaceVehicle &craft)
{
	auto &queue = craft.space_orders;
	if (!craft.manifest) { queue.blocked = SpaceOrderBlock::ProducerUnavailable; return false; }
	// Each outstanding descent still owns one return claim; the order cannot bypass it or create a second service allowance before actual recovery.
	if (craft.manifest->pending_returns != 0) { queue.blocked = SpaceOrderBlock::ReturnsPending; return false; }
	// Return does not top up a partly used budget. Existing native onboard service becomes due only when exhausted, and only its completed real work restores allowance.
	if (craft.manifest->returns_remaining != 0) return true;
	const auto &target = queue.entries[queue.cursor].target;
	AutoRestoreBackup actor(_current_company, craft.owner);
	CommandCost result;
	if (target.kind == SpaceOrderTargetKind::Structure) {
		result = CmdServiceOrbitalProbe(DoCommandFlag::Execute, SpaceStructureID{static_cast<uint16_t>(target.id)}, target.generation,
			craft.index, craft.generation, OrbitalServiceControl::Start);
	} else {
		result = CmdServiceLaunchProbe(DoCommandFlag::Execute, craft.index, craft.generation, false);
	}
	// Missing parts, absent power or an occupied berth retain this current action. The existing producer owns all countdowns, paused jobs, packet consumption and once-only renewal.
	queue.blocked = result.Failed() ? SpaceOrderBlock::ServiceRequired : SpaceOrderBlock::None;
	return false;
}
