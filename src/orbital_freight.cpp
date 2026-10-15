/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_freight.cpp Atomic whole-label admission over existing rocket and module packet owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group11.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Public Handoff Contract.
 */
#include "stdafx.h"
#include "orbital_freight.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_approach.h"
#include "space_structure.h"
#include "company_base.h"
#include "hub_cargo.h"
#include "safeguards.h"

/** Transient quantity/fragment views authorize one complete move; they own no cargo and are never saved. */
struct OrbitalFreightAdmission {
	std::array<uint64_t, NUM_CARGO> quantities{};
	std::array<size_t, NUM_CARGO> fragments{};
};

/** @param stock Actual station-style packet owner. @return Conservative fragment count across every routing bucket. */
static size_t CountOrbitalFreightFragments(const StationCargoList &stock)
{
	size_t count = 0;
	// Each routing bucket consumes native reference capacity even when quantities are small; possible destination merges never enlarge this bound.
	for (const auto &[hop, packets] : *stock.Packets()) { (void)hop; count += packets.size(); }
	return count;
}

/** Admit all existing source compartments and destination labels before the first packet moves.
 * @param rocket Exact native sender with its original compartments.
 * @param destination Generation-qualified actual module.
 * @param require_berth Whether physical arrival must already belong to this exact saved flight.
 * @param plan Receives only transient all-label quantity/fragment totals.
 * @return Whole native flight admission or the reason all owners must remain unchanged.
 */
static SpaceFlightBlock PlanOrbitalModuleFreight(const SpaceVehicle &rocket, SpaceStructureRef destination, bool require_berth, OrbitalFreightAdmission &plan)
{
	const auto *site = ResolveSpaceStructure(destination);
	// Native owner/role/identity admission excludes ordinary vehicles, stale destinations and another company's inventory before any physical lookup can grant capacity.
	if (site == nullptr || !Company::IsValidID(rocket.owner) || site->state.owner != rocket.owner || GetSpaceVehicleModel(rocket) != 14 || !rocket.manifest || rocket.vehstatus.Test(VehState::Crashed)) return SpaceFlightBlock::TargetLost;
	// Only actual commissioned freight modules supply capacity; a paid frame, lost dependency or unsupported later recipe remains inert.
	if (!IsOrbitalModuleOperational(*site) || GetOrbitalModuleStockLimit(site->state.kind) == 0) return SpaceFlightBlock::ReceiverBusy;
	const auto &manifest = *rocket.manifest;
	// Direct freight does not deploy a passenger, unpack paid pods or service a hull; those existing owners retain their separate explicit operations.
	if (manifest.passenger != SpaceCraftRef{} || !manifest.pods.empty() || manifest.service_ticks != 0) return SpaceFlightBlock::ReceiverBusy;
	if (require_berth) {
		const SpaceOrderTarget target{SpaceOrderTargetKind::Structure, destination.structure.base(), destination.generation};
		// A coincident position cannot unload: only this saved in-Space module flight and its actual shared admitted berth authorize arrival.
		if (rocket.construction.placement != SpacePlacement::InTransit || !rocket.transition || !IsSpaceModuleFreightApproach(*rocket.transition, target) || !GetSpaceStructureBerth(rocket, *site)) return SpaceFlightBlock::ReceiverBusy;
	}
	const uint freight = rocket.cargo.TotalCount();
	if (freight != 0) {
		// Freight must remain wholly in the native sender's available Keep owner; reserved/staged slices cannot become a second module delivery.
		if (!IsValidCargoType(rocket.cargo_type) || !HubCargoTransfer::CanMove(rocket.cargo, freight)) return SpaceFlightBlock::ReceiverBusy;
		plan.quantities[rocket.cargo_type] += freight; plan.fragments[rocket.cargo_type] += rocket.cargo.Packets()->size();
	}
	// RFMT/ELEC are separate physical compartments. Recovery propellant stays aboard and is never counted as deliverable module stock.
	for (uint8_t index = 1; index < SPACE_SUPPLY_COUNT; ++index) {
		const auto &stock = manifest.supplies[index]; const uint quantity = stock.TotalCount();
		if (quantity == 0) continue;
		const auto cargo = GetSpaceSupplyCargo(static_cast<SpaceSupply>(index));
		// Missing labels or partially available station-style reservations refuse the entire multi-compartment transfer.
		if (!IsValidCargoType(cargo) || stock.ReservedCount() != 0 || stock.AvailableCount() != quantity || !HubCargoTransfer::CanMove(stock, quantity)) return SpaceFlightBlock::ReceiverBusy;
		plan.quantities[cargo] += quantity; plan.fragments[cargo] += CountOrbitalFreightFragments(stock);
	}
	bool useful = false;
	// Aggregate compartments sharing a label before checking capacity: a full later part must never allow an earlier freight packet to leave first.
	for (CargoType cargo : EnumRange(NUM_CARGO)) {
		if (plan.quantities[cargo] == 0) continue;
		useful = true; const auto &stock = site->module->stock[cargo]; const uint64_t capacity = GetOrbitalModuleCargoCapacity(*site, cargo);
		const size_t fragments = CountOrbitalFreightFragments(stock);
		if (stock.ReservedCount() != 0) return SpaceFlightBlock::ReceiverBusy;
		// Both native quantity and reference bounds must fit as a whole; conservative fragment admission requires no packet allocation during execution.
		if (stock.TotalCount() > capacity || plan.quantities[cargo] > capacity - stock.TotalCount() || fragments > 64 || plan.fragments[cargo] > 64 - fragments) return SpaceFlightBlock::ReceiverFull;
	}
	return useful ? SpaceFlightBlock::None : SpaceFlightBlock::ReceiverFull;
}

/** @copydoc GetOrbitalModuleFreightBlock */
SpaceFlightBlock GetOrbitalModuleFreightBlock(const SpaceVehicle &rocket, SpaceStructureRef destination, bool require_berth)
{
	OrbitalFreightAdmission plan; return PlanOrbitalModuleFreight(rocket, destination, require_berth, plan);
}

/** @copydoc CommitOrbitalModuleFreight */
bool CommitOrbitalModuleFreight(SpaceVehicle &rocket, SpaceStructureRef destination)
{
	OrbitalFreightAdmission plan;
	if (PlanOrbitalModuleFreight(rocket, destination, true, plan) != SpaceFlightBlock::None) return false;
	auto &module = *ResolveSpaceStructure(destination)->module;
	const uint freight = rocket.cargo.TotalCount();
	if (freight != 0) {
		const bool moved = HubCargoTransfer::Move(rocket.cargo, module.stock[rocket.cargo_type], freight, rocket.tile); assert(moved); (void)moved;
	}
	// Every compartment was admitted together; whole existing packets move without a split, new cargo, income, material debit or utility mutation.
	for (uint8_t index = 1; index < SPACE_SUPPLY_COUNT; ++index) {
		auto &stock = rocket.manifest->supplies[index]; const uint quantity = stock.TotalCount();
		if (quantity == 0) continue;
		const bool moved = HubCargoTransfer::Move(stock, module.stock[GetSpaceSupplyCargo(static_cast<SpaceSupply>(index))], quantity); assert(moved); (void)moved;
	}
	return true;
}
