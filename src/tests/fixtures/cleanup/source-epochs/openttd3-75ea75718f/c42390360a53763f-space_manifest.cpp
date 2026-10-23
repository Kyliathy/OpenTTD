/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_manifest.cpp Actual native spacecraft fuel/service custody.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups2C/8C.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1/1B; exclusive Core service and packet custody.
 */
#include "stdafx.h"
#include "space_manifest.h"
#include "orbital_service.h"
#include "space_pod.h"
#include "space_transition.h"
#include "space_vehicle.h"
#include "space_structure.h"
#include "launch_operations.h"
#include "launch_center_cmd.h"
#include "multilayer_space_content.h"
#include "station_base.h"
#include "company_base.h"
#include "company_func.h"
#include "hub_cargo.h"
#include "command_func.h"
#include "map_func.h"
#include "window_func.h"
#include <set>
#include "safeguards.h"

CargoType GetSpaceSupplyCargo(SpaceSupply supply)
{
	static constexpr CargoLabel labels[] = {CargoLabel{"PROP"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}};
	return supply < SpaceSupply::End ? GetCargoTypeByLabel(labels[to_underlying(supply)]) : INVALID_CARGO;
}
uint16_t GetSpaceSupplyCapacity(uint16_t model, SpaceSupply supply)
{
	const auto &b = GetMultiLayerSpaceBudget();
	if (supply >= SpaceSupply::End) return 0;
	if (model == 14) return supply == SpaceSupply::Propellant ? b.fuel_hold : supply == SpaceSupply::RefinedMetal ? b.launch_metal_cap : b.launch_electronics_cap;
	if (model == 15 || model == 19 || model == 20) return supply == SpaceSupply::Propellant ? 0 : supply == SpaceSupply::RefinedMetal ? b.probe_metal_cap : b.probe_electronics_cap;
	return 0;
}
SpaceManifest &GetOrCreateSpaceManifest(SpaceVehicle &v)
{
	if (!v.manifest) {
		v.manifest = std::make_unique<SpaceManifest>();
	}
	return *v.manifest;
}
bool HasSpaceManifestAssets(const SpaceVehicle &v)
{
	if (!v.manifest) return false;
	const auto &m = *v.manifest;
	if (m.passenger != SpaceCraftRef{} || m.carrier != SpaceCraftRef{} || m.service_ticks != 0 || m.pending_returns != 0 || !m.pods.empty()) return true;
	for (const auto &stock : m.supplies) if (stock.TotalCount() != 0) return true;
	return false;
}
static uint64_t OwnPayloadMass(const SpaceVehicle &v)
{
	if (!IsValidCargoType(v.cargo_type)) return UINT64_MAX;
	uint64_t mass = uint64_t{v.cargo.TotalCount()} * CargoSpec::Get(v.cargo_type)->weight;
	if (v.manifest) for (uint i = 1; i < SPACE_SUPPLY_COUNT; ++i) {
		const auto cargo = GetSpaceSupplyCargo(static_cast<SpaceSupply>(i)); if (!IsValidCargoType(cargo)) return UINT64_MAX;
		mass += uint64_t{v.manifest->supplies[i].TotalCount()} * CargoSpec::Get(cargo)->weight;
	}
	if (v.manifest) mass += v.manifest->pods.size() * GetSpaceConstructionDefinition(SpaceBuildKind::Pod)->dry_mass_sixteenths;
	return mass;
}
uint64_t GetSpacePayloadMass(const SpaceVehicle &v)
{
	uint64_t mass = OwnPayloadMass(v);
	if (mass == UINT64_MAX) return mass;
	if (v.manifest && v.manifest->passenger != SpaceCraftRef{}) {
		const auto *probe = ResolveSpaceCraft(v.manifest->passenger);
		if (probe == nullptr) return UINT64_MAX;
		const auto *d = GetSpaceVehicleConstruction(GetSpaceVehicleModel(*probe));
		const auto payload = OwnPayloadMass(*probe);
		if (d == nullptr || payload == UINT64_MAX) return UINT64_MAX;
		mass += d->dry_mass_sixteenths + payload;
	}
	return mass;
}
uint16_t GetSpaceFreightCapacity(uint16_t model, CargoType cargo)
{
	if (!IsValidCargoType(cargo) || CargoSpec::Get(cargo)->weight == 0) return 0;
	if (model != 14 && model != 15 && model != 20) return 0;
	const auto mass = model == 14 ? GetMultiLayerSpaceBudget().rocket_payload_sixteenths : GetMultiLayerSpaceBudget().probe_hold_sixteenths;
	return static_cast<uint16_t>(mass / CargoSpec::Get(cargo)->weight);
}
CommandCost CmdMoveLaunchFreight(DoCommandFlags flags, StationID station, uint64_t center_generation, VehicleID vehicle, uint64_t generation, CargoType cargo, uint16_t quantity, bool unload)
{
	auto *v = ResolveSpaceCraft({vehicle, generation}); auto *center = ResolveLaunchCenter({station, center_generation});
	if (v == nullptr || center == nullptr || !IsValidCargoType(cargo) || quantity == 0 || v->owner != _current_company || center->owner != v->owner ||
		v->construction.placement != SpacePlacement::SurfaceStored || v->construction.home != GetLaunchCenterReference(*center) || (v->manifest && v->manifest->service_ticks != 0)) return CMD_ERROR;
	const auto model = GetSpaceVehicleModel(*v); const auto label = CargoSpec::Get(cargo)->label;
	if (model != 14 && model != 20) return CMD_ERROR;
	if (model == 20 && label != CargoLabel{"STEL"} && label != CargoLabel{"GOOD"}) return CMD_ERROR;
	if (model == 14 && !IsLaunchSupplyCargo(cargo)) return CMD_ERROR;
	if (v->cargo.TotalCount() != 0 && v->cargo_type != cargo) return CMD_ERROR;
	if (v->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) != v->cargo.TotalCount()) return CMD_ERROR;
	if (unload) {
		if (v->cargo_type != cargo || quantity > GetLaunchStockHeadroom(*center, cargo) || !HubCargoTransfer::CanMove(v->cargo, quantity)) return CMD_ERROR;
	} else {
		const auto capacity = GetSpaceFreightCapacity(model, cargo);
		if (v->cargo.TotalCount() > capacity || quantity > capacity - v->cargo.TotalCount() || !center->goods[cargo].HasData() || !HubCargoTransfer::CanMove(center->goods[cargo].GetData().cargo, quantity)) return CMD_ERROR;
		if (model == 14) {
			const auto mass = GetSpacePayloadMass(*v), extra = uint64_t{quantity} * CargoSpec::Get(cargo)->weight;
			if (mass > GetMultiLayerSpaceBudget().rocket_payload_sixteenths || extra > GetMultiLayerSpaceBudget().rocket_payload_sixteenths - mass) return CMD_ERROR;
		}
	}
	if (flags.Test(DoCommandFlag::Execute)) {
		v->cargo_type = cargo; v->cargo_cap = GetSpaceFreightCapacity(model, cargo);
		auto &stock = center->goods[cargo].GetOrCreateData().cargo;
		const bool moved = unload ? HubCargoTransfer::Move(v->cargo, stock, quantity, v->tile) : HubCargoTransfer::Move(stock, v->cargo, quantity, v->tile);
		assert(moved); (void)moved; v->MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost{};
}
static void PutProbeInStorage(SpaceVehicle &v, Station &center)
{
	v.construction.placement = SpacePlacement::SurfaceStored; v.construction.paused = false;
	v.construction.surface_tile = GetLaunchFacilityZone(center.launch_center->tile, center.launch_center->rotation, LaunchFacilityZone::Storage)->tile;
	v.position = v.home = *GetLaunchCenterPlaneReference(center.launch_center->tile); v.UpdateSpacePosition(); SyncSpacePods(v); v.MarkDirty();
}
void SyncSpacePassenger(SpaceVehicle &carrier)
{
	SyncSpacePods(carrier);
	if (!carrier.manifest) return;
	auto *v = ResolveSpaceCraft(carrier.manifest->passenger);
	if (v == nullptr || !v->manifest || v->manifest->carrier != GetSpaceCraftReference(carrier)) return;
	v->construction.surface_tile = carrier.construction.surface_tile;
	v->position = carrier.position; v->home = carrier.home; v->UpdateSpacePosition(); v->MarkDirty();
	SyncSpacePods(*v);
}
void DetachSpaceManifestAtLostCenter(SpaceVehicle &v)
{
	if (!v.manifest) return;
	auto &m = *v.manifest;
	if (auto *carrier = ResolveSpaceCraft(m.carrier); carrier != nullptr && carrier->manifest && carrier->manifest->passenger == GetSpaceCraftReference(v)) carrier->manifest->passenger = {};
	if (auto *probe = ResolveSpaceCraft(m.passenger); probe != nullptr && probe->manifest && probe->manifest->carrier == GetSpaceCraftReference(v)) {
		probe->manifest->carrier = {}; probe->construction.home = {}; probe->construction.placement = SpacePlacement::Unhomed; probe->construction.paused = true;
		if (probe->manifest->service_ticks != 0) probe->manifest->service_paused = true;
		probe->MarkDirty();
	}
	m.carrier = {}; m.passenger = {}; if (m.service_ticks != 0) m.service_paused = true;
}
void ForgetSpaceManifest(SpaceVehicle &v, bool cleaning)
{
	if (!v.manifest) return;
	if (cleaning) { for (auto &stock : v.manifest->supplies) stock.OnCleanPool(); return; }
	ForgetSpacePodCraft(v);
	if (auto *parent = ResolveSpaceCraft(v.manifest->carrier); parent != nullptr && parent->manifest && parent->manifest->passenger == GetSpaceCraftReference(v)) parent->manifest->passenger = {};
	if (auto *probe = ResolveSpaceCraft(v.manifest->passenger); probe != nullptr && probe->manifest && probe->manifest->carrier == GetSpaceCraftReference(v)) {
		probe->manifest->carrier = {};
		if (GetSpaceVehicleLayer(v) == MapLayer::Space) {
			probe->construction.placement = SpacePlacement::Orbital; probe->construction.paused = false; probe->construction.deployed = true;
			probe->vehstatus.Reset({VehState::Hidden, VehState::Stopped}); probe->UpdateSpacePosition(); SyncSpacePods(*probe); probe->MarkDirty(); return;
		}
		auto *center = ResolveLaunchCenter(probe->construction.home);
		if (center != nullptr && center->owner == probe->owner) PutProbeInStorage(*probe, *center);
		else { probe->construction.home = {}; probe->construction.placement = SpacePlacement::Unhomed; probe->construction.paused = true; }
	}
}
CommandCost CmdMoveSpaceSupply(DoCommandFlags flags, StationID station, uint64_t center_generation, VehicleID vehicle, uint64_t generation, uint8_t index, uint16_t quantity, bool unload)
{
	auto *v = ResolveSpaceCraft({vehicle, generation}); auto *center = ResolveLaunchCenter({station, center_generation});
	if (v == nullptr || center == nullptr || index >= SPACE_SUPPLY_COUNT || quantity == 0 || v->owner != _current_company || center->owner != v->owner ||
		v->construction.placement != SpacePlacement::SurfaceStored || v->construction.home != GetLaunchCenterReference(*center) || (v->manifest && v->manifest->service_ticks != 0)) return CMD_ERROR;
	const auto supply = static_cast<SpaceSupply>(index); const auto cargo = GetSpaceSupplyCargo(supply);
	if (!IsValidCargoType(cargo)) return CMD_ERROR;
	const uint held = v->manifest ? v->manifest->supplies[index].TotalCount() : 0;
	if (unload) {
		if (!v->manifest || quantity > GetLaunchStockHeadroom(*center, cargo) || !HubCargoTransfer::CanMove(v->manifest->supplies[index], quantity)) return CMD_ERROR;
	} else {
		const auto capacity = GetSpaceSupplyCapacity(GetSpaceVehicleModel(*v), supply);
		if (held > capacity || quantity > capacity - held || !center->goods[cargo].HasData() || !HubCargoTransfer::CanMove(center->goods[cargo].GetData().cargo, quantity)) return CMD_ERROR;
		if (GetSpaceVehicleModel(*v) == 14 && supply != SpaceSupply::Propellant) {
			const auto mass = GetSpacePayloadMass(*v); const auto extra = uint64_t{quantity} * CargoSpec::Get(cargo)->weight;
			if (mass > GetMultiLayerSpaceBudget().rocket_payload_sixteenths || extra > GetMultiLayerSpaceBudget().rocket_payload_sixteenths - mass) return CMD_ERROR;
		}
	}
	if (flags.Test(DoCommandFlag::Execute)) {
		auto &m = GetOrCreateSpaceManifest(*v); auto &stock = center->goods[cargo].GetOrCreateData().cargo;
		const bool moved = unload ? HubCargoTransfer::Move(m.supplies[index], stock, quantity) : HubCargoTransfer::Move(stock, m.supplies[index], quantity);
		assert(moved); (void)moved; v->MarkDirty(); SetWindowClassesDirty(WindowClass::StationView); SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost{};
}
CommandCost CmdCarryLaunchProbe(DoCommandFlags flags, VehicleID rocket_id, uint64_t rocket_generation, VehicleID probe_id, uint64_t probe_generation, bool unload)
{
	auto *rocket = ResolveSpaceCraft({rocket_id, rocket_generation}); auto *probe = ResolveSpaceCraft({probe_id, probe_generation});
	if (rocket == nullptr || probe == nullptr || rocket == probe || rocket->owner != _current_company || probe->owner != rocket->owner || GetSpaceVehicleModel(*rocket) != 14 ||
		(GetSpaceVehicleModel(*probe) != 15 && GetSpaceVehicleModel(*probe) != 19 && GetSpaceVehicleModel(*probe) != 20) ||
		rocket->construction.placement != SpacePlacement::SurfaceStored || rocket->construction.home != probe->construction.home ||
		(rocket->manifest && rocket->manifest->service_ticks != 0) || (probe->manifest && probe->manifest->service_ticks != 0)) return CMD_ERROR;
	auto *center = ResolveLaunchCenter(rocket->construction.home); if (center == nullptr || center->owner != rocket->owner) return CMD_ERROR;
	if (unload) {
		if (!rocket->manifest || !probe->manifest || rocket->manifest->passenger != GetSpaceCraftReference(*probe) || probe->manifest->carrier != GetSpaceCraftReference(*rocket) || probe->construction.placement != SpacePlacement::Carried) return CMD_ERROR;
	} else {
		if (probe->construction.placement != SpacePlacement::SurfaceStored || (rocket->manifest && rocket->manifest->passenger != SpaceCraftRef{}) ||
			(probe->manifest && (probe->manifest->carrier != SpaceCraftRef{} || probe->manifest->passenger != SpaceCraftRef{}))) return CMD_ERROR;
		const auto own = GetSpacePayloadMass(*rocket), cargo = OwnPayloadMass(*probe);
		const auto dry = GetSpaceVehicleConstruction(GetSpaceVehicleModel(*probe))->dry_mass_sixteenths;
		if (own == UINT64_MAX || cargo == UINT64_MAX || own + cargo + dry > GetMultiLayerSpaceBudget().rocket_payload_sixteenths) return CMD_ERROR;
	}
	if (flags.Test(DoCommandFlag::Execute)) {
		auto &r = GetOrCreateSpaceManifest(*rocket); auto &p = GetOrCreateSpaceManifest(*probe);
		if (unload) { r.passenger = {}; p.carrier = {}; PutProbeInStorage(*probe, *center); }
		else { r.passenger = GetSpaceCraftReference(*probe); p.carrier = GetSpaceCraftReference(*rocket); probe->construction.placement = SpacePlacement::Carried; SyncSpacePassenger(*rocket); }
		SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost{};
}
/** Service through the existing Earth/onboard owner; a reserved Core job must be controlled at its own berth.
 * @param flags Estimate or execute. @param vehicle Native probe ID. @param generation Exact hull incarnation.
 * @param cancel True pauses the existing service. @return Complete service admission/control or unchanged refusal.
 */
CommandCost CmdServiceLaunchProbe(DoCommandFlags flags, VehicleID vehicle, uint64_t generation, bool cancel)
{
	// A Core reservation is the sole service owner until completed or explicitly cancelled there.
	if (const auto *craft = ResolveSpaceCraft({vehicle, generation}); craft != nullptr && HasOrbitalServiceAssignment(*craft)) return CMD_ERROR;
	auto *v = ResolveSpaceCraft({vehicle, generation}); if (v == nullptr || v->owner != _current_company || !v->manifest ||
		(v->construction.placement != SpacePlacement::SurfaceStored && v->construction.placement != SpacePlacement::Orbital)) return CMD_ERROR;
	const auto model = GetSpaceVehicleModel(*v); if (model != 15 && model != 19 && model != 20) return CMD_ERROR;
	auto &m = *v->manifest;
	if (v->construction.placement == SpacePlacement::Orbital) {
		if (m.pending_returns != 0 || v->mission.phase != SpaceMissionPhase::Holding ||
			(v->space_orders.cursor < v->space_orders.entries.size() && !IsSpaceMiningServiceWait(v->space_orders))) return CMD_ERROR;
		if (cancel ? m.service_ticks == 0 || m.service_paused || m.service_location != SpaceServiceLocation::Onboard :
			(m.service_ticks != 0 && !m.service_paused) || m.returns_remaining != 0 || m.services_completed == UINT64_MAX || m.supplies[1].AvailableCount() < 1 || m.supplies[2].AvailableCount() < 1) return CMD_ERROR;
		if (flags.Test(DoCommandFlag::Execute)) {
			m.service_location = SpaceServiceLocation::Onboard; m.service_paused = cancel;
			if (!cancel && m.service_ticks == 0) m.service_ticks = GetMultiLayerSpaceBudget().service_ticks;
			v->MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly);
		}
		return CommandCost{};
	}
	auto *center = ResolveLaunchCenter(v->construction.home); if (center == nullptr || center->owner != v->owner || !center->launch_operations || m.pending_returns != 0) return CMD_ERROR;
	auto &op = *center->launch_operations;
	if (cancel) { if (m.service_ticks == 0 || m.service_paused || op.service != GetSpaceCraftReference(*v)) return CMD_ERROR; }
	else if ((m.service_ticks != 0 && !m.service_paused) || m.services_completed == UINT64_MAX || op.workshop != SpaceCraftRef{} || op.service != SpaceCraftRef{} || op.pod_workshop != SpacePodRef{} ||
		m.supplies[1].AvailableCount() < 1 || m.supplies[2].AvailableCount() < 1 || m.carrier != SpaceCraftRef{} || m.passenger != SpaceCraftRef{}) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		if (cancel) { op.service = {}; m.service_paused = true; PutProbeInStorage(*v, *center); }
		else {
			op.service = GetSpaceCraftReference(*v); if (m.service_ticks == 0) m.service_ticks = GetMultiLayerSpaceBudget().service_ticks; m.service_paused = false; m.service_location = SpaceServiceLocation::EarthWorkshop;
			v->construction.surface_tile = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Workshop)->tile; v->UpdateSpacePosition(); SyncSpacePods(*v);
		}
		v->MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost{};
}
bool IsSpaceServiceWorkTick(const SpaceVehicle &v)
{
	if (!v.manifest) return false;
	const auto &m = *v.manifest;
	if (m.service_ticks == 0 || m.service_paused || m.supplies[1].AvailableCount() < 1 || m.supplies[2].AvailableCount() < 1) return false;
	if (m.service_location == SpaceServiceLocation::Onboard) {
		return v.construction.placement == SpacePlacement::Orbital && v.mission.phase == SpaceMissionPhase::Holding && m.pending_returns == 0;
	}
	const auto *center = ResolveLaunchCenter(v.construction.home);
	return v.construction.placement == SpacePlacement::SurfaceStored && center != nullptr && center->owner == v.owner && center->launch_operations &&
		center->launch_operations->service == GetSpaceCraftReference(v);
}

SpaceFlightBlock GetSpaceResupplyBlock(const SpaceVehicle &rocket, SpaceCraftRef receiver)
{
	const auto *v = ResolveSpaceCraft(receiver);
	if (v == nullptr || v == &rocket || !Company::IsValidID(rocket.owner) || v->owner != rocket.owner || v->construction.placement != SpacePlacement::Orbital ||
		GetSpaceVehicleModel(rocket) != 14 || !rocket.manifest || rocket.manifest->passenger != SpaceCraftRef{}) return SpaceFlightBlock::TargetLost;
	const auto model = GetSpaceVehicleModel(*v);
	if (model != 15 && model != 19 && model != 20) return SpaceFlightBlock::TargetLost;
	// Incoming paid supplies retain their carrier while a Core owns the receiver's precise service berth.
	if (HasOrbitalServiceAssignment(*v) || v->mission.phase != SpaceMissionPhase::Holding || (v->manifest && v->manifest->service_ticks != 0) ||
		(v->space_orders.cursor < v->space_orders.entries.size() && !IsSpaceMiningServiceWait(v->space_orders))) return SpaceFlightBlock::ReceiverBusy;
	const auto &from = *rocket.manifest; const auto *to = v->manifest.get();
	const size_t pod_cap = GetMultiLayerSpaceBudget().starting_pods;
	const size_t held_pods = to == nullptr ? 0 : to->pods.size();
	if (held_pods > pod_cap || from.pods.size() > pod_cap - held_pods) return SpaceFlightBlock::ReceiverFull;
	bool payload = !from.pods.empty() || rocket.cargo.TotalCount() != 0;
	for (uint i = 1; i < SPACE_SUPPLY_COUNT; ++i) {
		const auto count = from.supplies[i].TotalCount(); const auto capacity = GetSpaceSupplyCapacity(model, static_cast<SpaceSupply>(i));
		const auto held = to == nullptr ? 0U : to->supplies[i].TotalCount();
		if (held > capacity || count > capacity - held || from.supplies[i].ReservedCount() != 0) return SpaceFlightBlock::ReceiverFull;
		payload |= count != 0;
	}
	if (!payload) return SpaceFlightBlock::ReceiverFull;
	for (const auto ref : from.pods) {
		const auto *pod = ResolveSpacePod(ref);
		if (pod == nullptr || pod->state.owner != rocket.owner || pod->state.custody != SpacePodCustody::Craft || pod->state.craft != GetSpaceCraftReference(rocket) ||
			std::count(from.pods.begin(), from.pods.end(), ref) != 1 || (to != nullptr && std::find(to->pods.begin(), to->pods.end(), ref) != to->pods.end())) return SpaceFlightBlock::TargetLost;
	}
	if (rocket.cargo.TotalCount() != 0) {
		if (!IsValidCargoType(rocket.cargo_type)) return SpaceFlightBlock::ReceiverFull;
		const auto cargo = rocket.cargo_type; const auto label = CargoSpec::Get(cargo)->label;
		const auto capacity = GetSpaceFreightCapacity(model, cargo);
		if (model != 20 || (label != CargoLabel{"STEL"} && label != CargoLabel{"GOOD"}) || (v->cargo.TotalCount() != 0 && v->cargo_type != cargo) ||
			v->cargo.TotalCount() > capacity || rocket.cargo.TotalCount() > capacity - v->cargo.TotalCount() ||
			rocket.cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) != rocket.cargo.TotalCount() ||
			v->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) != v->cargo.TotalCount()) return SpaceFlightBlock::ReceiverFull;
	}
	return SpaceFlightBlock::None;
}

bool CommitSpaceResupply(SpaceVehicle &rocket, SpaceCraftRef receiver)
{
	if (!rocket.transition || rocket.transition->phase != SpaceFlightPhase::OrbitalDelivery || rocket.transition->resupplied ||
		rocket.transition->destination != SpaceOrderTarget{SpaceOrderTargetKind::Craft, receiver.vehicle.base(), receiver.generation} ||
		GetSpaceResupplyBlock(rocket, receiver) != SpaceFlightBlock::None) return false;
	auto *v = ResolveSpaceCraft(receiver);
	if (SpaceDistanceSquared(rocket.position, v->position) > uint64_t{SPACE_Q16 / 4} * (SPACE_Q16 / 4)) return false;
	auto &from = *rocket.manifest; auto &to = GetOrCreateSpaceManifest(*v); // Old absence means zero stock/quota; delivered paid inputs create only this native compartment owner.
	to.pods.reserve(to.pods.size() + from.pods.size()); // Allocate vector headroom before physical owner changes.
	for (uint i = 1; i < SPACE_SUPPLY_COUNT; ++i) if (const auto count = from.supplies[i].TotalCount(); count != 0) {
		const bool moved = HubCargoTransfer::Move(from.supplies[i], to.supplies[i], count); assert(moved); (void)moved; // Whole lists require no packet splits.
	}
	if (const auto count = rocket.cargo.TotalCount(); count != 0) {
		v->cargo_type = rocket.cargo_type; v->cargo_cap = GetSpaceFreightCapacity(GetSpaceVehicleModel(*v), v->cargo_type);
		const auto moved = rocket.cargo.Shift(count, &v->cargo); assert(moved == count); (void)moved;
	}
	for (const auto ref : from.pods) { ResolveSpacePod(ref)->state.craft = receiver; to.pods.push_back(ref); }
	from.pods.clear(); SyncSpacePods(*v); v->MarkDirty(); rocket.MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly);
	return true;
}

void TickSpaceManifests()
{
	for (auto *v : SpaceVehicle::Iterate()) if (v->manifest) {
		auto &m = *v->manifest;
		if (++m.age_ticks == Ticks::CARGO_AGING_TICKS) { m.age_ticks = 0; for (auto &stock : m.supplies) HubCargoTransfer::Age(stock); }
		if (m.service_ticks == 0 || m.service_paused) continue;
		if (!IsSpaceServiceWorkTick(*v)) { m.service_paused = true; continue; }
		Station *center = m.service_location == SpaceServiceLocation::EarthWorkshop ? ResolveLaunchCenter(v->construction.home) : nullptr;
		if (--m.service_ticks == 0) {
			const bool metal = HubCargoTransfer::Consume(m.supplies[1], 1), electronics = HubCargoTransfer::Consume(m.supplies[2], 1); assert(metal && electronics); (void)metal; (void)electronics;
			++m.services_completed; m.returns_remaining = GetMultiLayerSpaceBudget().returns_per_service; m.service_location = SpaceServiceLocation::EarthWorkshop;
			if (center != nullptr) { center->launch_operations->service = {}; PutProbeInStorage(*v, *center); }
			SetWindowClassesDirty(WindowClass::LaunchAssembly);
		}
	}
}

std::vector<SpaceManifestRecord> CaptureSpaceManifests()
{
	std::vector<SpaceManifestRecord> result;
	for (const auto *v : SpaceVehicle::Iterate()) if (v->manifest) {
		const auto &m = *v->manifest; SpaceManifestRecord r;
		r.craft = GetSpaceCraftReference(*v); r.passenger = m.passenger; r.carrier = m.carrier; r.pods = m.pods;
		r.age_ticks = m.age_ticks; r.service_ticks = m.service_ticks; r.returns_remaining = m.returns_remaining; r.service_paused = m.service_paused;
		r.pending_returns = m.pending_returns; r.service_location = m.service_location;
		r.fuel_burned = m.fuel_burned; r.services_completed = m.services_completed;
		for (uint i = 0; i < SPACE_SUPPLY_COUNT; ++i) for (const auto &[hop, packets] : *m.supplies[i].Packets()) for (const auto *cp : packets) r.supplies[i].push_back({cp->index, hop});
		result.push_back(std::move(r));
	}
	return result;
}
bool ValidateSpaceManifestRecords(std::span<const SpaceManifestRecord> records)
{
	if (records.size() > MAX_SPACE_VEHICLES) return false;
	std::set<VehicleID> craft; std::set<uint64_t> generations; std::set<CargoPacketID> packets; std::set<SpacePodID> pods;
	auto ref = [](SpaceCraftRef r) { return r == SpaceCraftRef{} || (r.vehicle != VehicleID::Invalid() && r.generation != 0); };
	for (const auto &r : records) {
		if (r.craft == SpaceCraftRef{} || !ref(r.craft) || !ref(r.passenger) || !ref(r.carrier) || !craft.insert(r.craft.vehicle).second || !generations.insert(r.craft.generation).second ||
			r.passenger == r.craft || r.carrier == r.craft || (r.passenger != SpaceCraftRef{} && r.carrier != SpaceCraftRef{}) ||
			r.age_ticks >= Ticks::CARGO_AGING_TICKS || r.service_ticks > GetMultiLayerSpaceBudget().service_ticks ||
			r.returns_remaining > GetMultiLayerSpaceBudget().returns_per_service || r.pending_returns > r.returns_remaining || r.service_location >= SpaceServiceLocation::End ||
			(r.service_ticks == 0 && (r.service_paused || r.service_location != SpaceServiceLocation::EarthWorkshop)) ||
			(r.service_ticks != 0 && r.pending_returns != 0) ||
			(r.service_ticks != 0 && r.services_completed == UINT64_MAX)) return false;
		if (r.pods.size() > GetMultiLayerSpaceBudget().launch_pod_cap) return false;
		for (const auto ref : r.pods) if (ref.pod >= SpacePodID::End() || ref.generation == 0 || !pods.insert(ref.pod).second) return false;
		for (const auto &slot : r.supplies) {
			if (slot.size() > 128) return false;
			for (const auto &[packet, hop] : slot) if (packet == CargoPacketID::Invalid() || !packets.insert(packet).second || (hop != StationID::Invalid() && hop >= StationID::End())) return false;
		}
	}
	return true;
}
/** Audit native ordinary and orbital packet owners before accepting manifest custody.
 * @param owned Previously encountered physical packet identities, extended by each actual container.
 * @return Whether ordinary stations, vehicles, construction jobs and completed Core stock remain exclusive.
 */
static bool OtherPacketOwners(std::set<CargoPacketID> &owned)
{
	// Completed module stock and unfinished inputs are separate physical owners; neither may also appear in a spacecraft compartment.
	if (!InsertOrbitalPacketOwners(owned)) return false;
	auto add = [&](const auto &list) { for (const auto *cp : list) if (!owned.insert(cp->index).second) return false; return true; };
	for (const auto *station : Station::Iterate()) {
		for (CargoType cargo : EnumRange(NUM_CARGO)) if (station->goods[cargo].HasData()) {
			for (const auto &[hop, packets] : *station->goods[cargo].GetData().cargo.Packets()) { (void)hop; if (!add(packets)) return false; }
		}
		if (station->launch_hub) for (const auto &job : station->launch_hub->handling) for (const auto &[hop, packets] : *job->packets.Packets()) { (void)hop; if (!add(packets)) return false; }
	}
	for (const auto *v : Vehicle::Iterate()) if (!add(*v->cargo.Packets())) return false;
	return true;
}
static bool ManifestPackets(std::span<const SpaceManifestRecord> records)
{
	std::set<CargoPacketID> owned; if (!OtherPacketOwners(owned)) return false;
	for (const auto &r : records) {
		const auto *v = ResolveSpaceCraft(r.craft); if (v == nullptr || !Company::IsValidID(v->owner)) return false;
		const auto model = GetSpaceVehicleModel(*v);
		if (model != 14 && model != 15 && model != 19 && model != 20) return false;
		if (model == 14 && (r.carrier != SpaceCraftRef{} || r.service_ticks != 0 || r.returns_remaining != 0 || r.pending_returns != 0 || r.services_completed != 0)) return false;
		if (model != 14 && (r.passenger != SpaceCraftRef{} || r.fuel_burned != 0)) return false;
		for (uint i = 0; i < SPACE_SUPPLY_COUNT; ++i) {
			uint64_t count = 0;
			for (const auto &[id, hop] : r.supplies[i]) {
				(void)hop; const auto *cp = CargoPacket::GetIfValid(id);
				if (cp == nullptr || !owned.insert(id).second) return false; count += cp->Count();
			}
			if (count > GetSpaceSupplyCapacity(model, static_cast<SpaceSupply>(i))) return false;
			if (r.service_ticks != 0 && i != 0 && count < 1) return false;
		}
		auto find = [&](SpaceCraftRef craft) -> const SpaceManifestRecord * { for (const auto &candidate : records) if (candidate.craft == craft) return &candidate; return nullptr; };
		if (r.passenger != SpaceCraftRef{}) {
			const auto *child = find(r.passenger); const auto *probe = ResolveSpaceCraft(r.passenger);
			if (child == nullptr || probe == nullptr || child->carrier != r.craft || probe->owner != v->owner || GetSpaceVehicleModel(*probe) == 14) return false;
		}
		if (r.carrier != SpaceCraftRef{}) {
			const auto *parent = find(r.carrier); const auto *rocket = ResolveSpaceCraft(r.carrier);
			if (parent == nullptr || rocket == nullptr || parent->passenger != r.craft || rocket->owner != v->owner || GetSpaceVehicleModel(*rocket) != 14) return false;
		}
	}
	return true;
}
bool InstallLoadedSpaceManifests(std::span<const SpaceManifestRecord> records)
{
	if (!ValidateSpaceManifestRecords(records) || !ManifestPackets(records)) return false;
	for (const auto *v : SpaceVehicle::Iterate()) if (v->manifest) return false; // Fresh load only; replacing a live owner could destroy its packets.
	for (const auto &r : records) {
		auto *v = ResolveSpaceCraft(r.craft); v->manifest = std::make_unique<SpaceManifest>(); auto &m = *v->manifest;
		m.passenger = r.passenger; m.carrier = r.carrier; m.pods = r.pods; m.age_ticks = r.age_ticks; m.service_ticks = r.service_ticks; m.returns_remaining = r.returns_remaining;
		m.service_paused = r.service_paused; m.fuel_burned = r.fuel_burned; m.services_completed = r.services_completed;
		m.pending_returns = r.pending_returns; m.service_location = r.service_location;
		for (uint i = 0; i < SPACE_SUPPLY_COUNT; ++i) for (const auto &[id, hop] : r.supplies[i]) m.supplies[i].Append(CargoPacket::Get(id), hop);
	}
	// Append must not normalize a malformed save into different identities.
	const auto installed = CaptureSpaceManifests();
	if (installed.size() != records.size()) return false;
	for (size_t i = 0; i < records.size(); ++i) if (installed[i].supplies != records[i].supplies) return false;
	return true;
}
bool ValidateSpaceManifests()
{
	const auto records = CaptureSpaceManifests();
	if (!ValidateSpaceManifestRecords(records) || !ManifestPackets(records)) return false;
	for (const auto *v : SpaceVehicle::Iterate()) {
		if (!v->manifest) { if (v->construction.placement == SpacePlacement::Carried) return false; continue; }
		const auto &m = *v->manifest;
		if (v->construction.content_version == 0 && m.returns_remaining != 0 && m.services_completed == 0) return false;
		for (const auto &slot : m.supplies) if (slot.ReservedCount() != 0) return false;
		if (GetSpaceVehicleModel(*v) == 14 && GetSpacePayloadMass(*v) > GetMultiLayerSpaceBudget().rocket_payload_sixteenths) return false;
		if (m.carrier != SpaceCraftRef{}) {
			const auto *parent = ResolveSpaceCraft(m.carrier);
			if (v->construction.placement != SpacePlacement::Carried || (parent->construction.placement != SpacePlacement::SurfaceStored && parent->construction.placement != SpacePlacement::InTransit) ||
				v->construction.home != parent->construction.home || v->tile != parent->tile || v->x_pos != parent->x_pos || v->y_pos != parent->y_pos || v->z_pos != parent->z_pos ||
				v->position != parent->position || v->home != parent->home) return false;
		} else if (v->construction.placement == SpacePlacement::Carried) return false;
		if (m.service_ticks != 0) {
			if (m.service_location == SpaceServiceLocation::Onboard) {
				if (v->construction.placement != SpacePlacement::Orbital || v->mission.phase != SpaceMissionPhase::Holding || m.returns_remaining != 0 ||
					(v->space_orders.cursor < v->space_orders.entries.size() && !IsSpaceMiningServiceWait(v->space_orders))) return false;
			} else if (v->construction.placement == SpacePlacement::Unhomed) { if (!m.service_paused) return false; }
			else {
				const auto *center = ResolveLaunchCenter(v->construction.home);
				if (v->construction.placement != SpacePlacement::SurfaceStored || center == nullptr || center->owner != v->owner || !center->launch_operations ||
					(!m.service_paused && center->launch_operations->service != GetSpaceCraftReference(*v))) return false;
			}
		}
	}
	return true;
}
uint64_t ComputeSpaceManifestDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	auto add = [&](uint64_t value) { for (uint i = 0; i < 8; ++i) { hash ^= (value >> (i * 8)) & 255; hash *= 1099511628211ULL; } };
	auto ref = [&](SpaceCraftRef r) { add(r.vehicle.base()); add(r.generation); };
	for (const auto &r : CaptureSpaceManifests()) {
		ref(r.craft); ref(r.passenger); ref(r.carrier); add(r.age_ticks); add(r.service_ticks); add(r.service_paused); add(r.returns_remaining); add(r.fuel_burned); add(r.services_completed);
		add(r.pending_returns); add(to_underlying(r.service_location));
		add(r.pods.size()); for (const auto pod : r.pods) { add(pod.pod.base()); add(pod.generation); }
		for (const auto &slot : r.supplies) { add(slot.size()); for (const auto &[id, hop] : slot) {
			const auto *cp = CargoPacket::Get(id); add(id.base()); add(hop.base()); add(cp->Count()); add(cp->GetPeriodsInTransit()); add(static_cast<int64_t>(cp->GetFeederShare()));
			add(cp->GetSourceTile().base()); add(cp->GetFirstStation().base()); add(to_underlying(cp->GetSource().type)); add(cp->GetSource().id);
		} }
	}
	return hash;
}
