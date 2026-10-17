/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_manifest.cpp Actual native spacecraft fuel/service custody.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups2C/8C.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1/1B; exclusive Core service and packet custody.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B; reached Return/Service can retain its own actual onboard work.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CB; actual heavy pod/part resupply grants no service allowance or progress.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; pending heavy returns require their own source-era descent producer.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CC; the original onboard timer consumes real per-model service inputs once.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12; hauler freight uses canonical cargo refits and original physical mass.
 */
#include "stdafx.h"
#include "space_mining_tuning.h"
#include "space_manifest.h"
#include "space_order_actions.h"
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
#include "openttd.h"
#include <set>
#include "safeguards.h"

/** @copydoc GetSpaceSupplyCargo */
CargoType GetSpaceSupplyCargo(SpaceSupply supply)
{
	static constexpr CargoLabel labels[] = {CargoLabel{"PROP"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}};
	return supply < SpaceSupply::End ? GetCargoTypeByLabel(labels[to_underlying(supply)]) : INVALID_CARGO;
}
/** @copydoc GetSpaceSupplyCapacity */
uint16_t GetSpaceSupplyCapacity(uint16_t model, SpaceSupply supply)
{
	const auto &b = GetMultiLayerSpaceBudget();
	if (supply >= SpaceSupply::End) return 0;
	if (model == 14) return supply == SpaceSupply::Propellant ? b.fuel_hold : supply == SpaceSupply::RefinedMetal ? b.launch_metal_cap : b.launch_electronics_cap;
	// Every delivered orbital role uses the existing bounded service stores; manufacturing a heavy hull does not grant free packets or introduce its later mission tuning.
	if (model == 15 || model == 16 || model == 17 || model == 19 || model == 20 || model == SPACE_HEAVY_MINER_MODEL) return supply == SpaceSupply::Propellant ? 0 : supply == SpaceSupply::RefinedMetal ? b.probe_metal_cap : b.probe_electronics_cap;
	return 0;
}
/** @copydoc GetSpaceServiceTuning */
std::optional<SpaceServiceTuning> GetSpaceServiceTuning(uint16_t model)
{
	if (const auto *miner = GetSpaceMiningTuning(model)) return SpaceServiceTuning{miner->service_ticks, {miner->service_metal, miner->service_electronics}, miner->returns_per_service};
	// Delivered survey/construction probes retain their original bill; service-capacity compartments alone never grant construction16, hauler17 or a rocket a probe-service producer.
	if (model == 19 || model == 20) return SpaceServiceTuning{GetMultiLayerSpaceBudget().service_ticks, {1, 1}, GetMultiLayerSpaceBudget().returns_per_service};
	return std::nullopt;
}
/** @copydoc IsSpaceHeavyResupplyStateSupported */
bool IsSpaceHeavyResupplyStateSupported(uint16_t model, const SpaceManifestRecord &record, bool heavy_resupply, bool heavy_descents, bool heavy_service)
{
	if (model != SPACE_HEAVY_MINER_MODEL) return true;
	// A later descent capability cannot precede its real supply producer; actual source versions always advance these gates in that order.
	if ((heavy_descents && !heavy_resupply) || (heavy_service && !heavy_descents)) return false;
	// Each saved producer requires its own delivered source era; old supply/descent files cannot acquire future work or a completed service allowance.
	if ((!heavy_descents && record.pending_returns != 0) || (!heavy_service && (record.service_ticks != 0 || record.services_completed != 0))) return false;
	const auto service = GetSpaceServiceTuning(model);
	// The factory-only heavy's active manifest timer belongs solely to exhausted onboard service, not an invented Earth workshop or a partly used return top-up.
	if (record.service_ticks != 0 && (!service || record.service_ticks > service->ticks || record.service_location != SpaceServiceLocation::Onboard || record.returns_remaining != 0 || record.pending_returns != 0)) return false;
	if (heavy_resupply) return true;
	// Commissioning/mining-only sources never owned heavy shells or paid loose parts; a forged older header cannot grant that later capability.
	if (!record.pods.empty()) return false;
	// Every future compartment reference represents real later supply, including an invalid attempted fuel compartment; absence alone preserves genuine older depleted orbital allowance.
	for (const auto &slot : record.supplies) if (!slot.empty()) return false;
	return true;
}
/** @copydoc GetOrCreateSpaceManifest */
SpaceManifest &GetOrCreateSpaceManifest(SpaceVehicle &v)
{
	if (!v.manifest) {
		v.manifest = std::make_unique<SpaceManifest>();
	}
	return *v.manifest;
}
/** @copydoc HasSpaceManifestAssets */
bool HasSpaceManifestAssets(const SpaceVehicle &v)
{
	if (!v.manifest) return false;
	const auto &m = *v.manifest;
	// Referenced hulls, work, pending descents and shells retain obligations even when every loose compartment is empty; fleet disposal must preserve those owners.
	if (m.passenger != SpaceCraftRef{} || m.carrier != SpaceCraftRef{} || m.service_ticks != 0 || m.pending_returns != 0 || !m.pods.empty()) return true;
	// Any actual physical compartment blocks empty-asset disposal, regardless of its cargo label or future service use.
	for (const auto &stock : m.supplies) if (stock.TotalCount() != 0) return true;
	return false;
}
/** @param v Actual native hull. @return Its own physical freight/parts/shell mass, excluding a separate paid passenger, or UINT64_MAX for invalid cargo. */
static uint64_t OwnPayloadMass(const SpaceVehicle &v)
{
	if (!IsValidCargoType(v.cargo_type)) return UINT64_MAX;
	uint64_t mass = uint64_t{v.cargo.TotalCount()} * CargoSpec::Get(v.cargo_type)->weight;
	// Loose service parts and shells consume payload capacity; propellant stays in its separately bounded fuel compartment.
	if (v.manifest) for (uint i = 1; i < SPACE_SUPPLY_COUNT; ++i) {
		const auto cargo = GetSpaceSupplyCargo(static_cast<SpaceSupply>(i)); if (!IsValidCargoType(cargo)) return UINT64_MAX;
		mass += uint64_t{v.manifest->supplies[i].TotalCount()} * CargoSpec::Get(cargo)->weight;
	}
	if (v.manifest) mass += v.manifest->pods.size() * GetSpaceConstructionDefinition(SpaceBuildKind::Pod)->dry_mass_sixteenths;
	return mass;
}
/** @copydoc GetSpacePayloadMass */
uint64_t GetSpacePayloadMass(const SpaceVehicle &v)
{
	uint64_t mass = OwnPayloadMass(v);
	if (mass == UINT64_MAX) return mass;
	// A carried paid body and all of its actual assets share the rocket's physical payload limit, without cloning its manifest.
	if (v.manifest && v.manifest->passenger != SpaceCraftRef{}) {
		const auto *probe = ResolveSpaceCraft(v.manifest->passenger);
		if (probe == nullptr) return UINT64_MAX;
		const auto *d = GetSpaceVehicleConstruction(GetSpaceVehicleModel(*probe));
		const auto payload = OwnPayloadMass(*probe);
		// An unknown recipe or invalid nested payload cannot be treated as a weightless passenger during departure admission.
		if (d == nullptr || payload == UINT64_MAX) return UINT64_MAX;
		mass += d->dry_mass_sixteenths + payload;
	}
	return mass;
}
/** @copydoc GetSpaceFreightCapacity */
uint16_t GetSpaceFreightCapacity(uint16_t model, CargoType cargo)
{
	// Freight must have a real physical mass; invalid or weightless cargo cannot acquire an unbounded hold through division.
	if (!IsValidCargoType(cargo) || CargoSpec::Get(cargo)->weight == 0) return 0;
	// Only the declared freight-carrying roles receive the common mass bound; survey and construction-only heavy hulls receive no freight allowance.
	if (model != 14 && model != 15 && model != 17 && model != 20 && model != SPACE_HEAVY_MINER_MODEL) return 0;
	if (model == SPACE_HEAVY_MINER_MODEL || model == 17) {
		const auto capabilities = GetMultiLayerVehicleCapabilities();
		const auto definition = std::ranges::find(capabilities, model, &LayerVehicleCapability::model_id);
		// Heavy miners keep their mineral refits and haulers keep their published general-freight labels. A generic packet move cannot grant either role an undeclared cargo merely because it fits by mass.
		if (definition == capabilities.end() || std::ranges::find(definition->cargo_labels, CargoSpec::Get(cargo)->label) == definition->cargo_labels.end()) return 0;
	}
	const auto *miner = GetSpaceMiningTuning(model);
	const auto mass = model == 14 ? GetMultiLayerSpaceBudget().rocket_payload_sixteenths : miner != nullptr ? miner->hold_sixteenths : GetMultiLayerSpaceBudget().probe_hold_sixteenths;
	return static_cast<uint16_t>(mass / CargoSpec::Get(cargo)->weight);
}
/** Move actual whole freight between an owned Surface facility and its same-center physical hull.
 * @param flags Estimate or execute. @param station Actual facility ID. @param center_generation Exact center incarnation.
 * @param vehicle Actual hull ID. @param generation Exact hull incarnation. @param cargo Actual physical runtime cargo. @param quantity Positive whole units.
 * @param unload True returns freight to Surface stock; false loads actual available stock.
 * @return Complete role/custody/mass/headroom admission or unchanged refusal; crossing compartments grants no freight income.
 */
CommandCost CmdMoveLaunchFreight(DoCommandFlags flags, StationID station, uint64_t center_generation, VehicleID vehicle, uint64_t generation, CargoType cargo, uint16_t quantity, bool unload)
{
	auto *v = ResolveSpaceCraft({vehicle, generation}); auto *center = ResolveLaunchCenter({station, center_generation});
	// Only an actual same-owner stored hull at its real Surface facility may move whole freight; active service retains its physical compartments.
	if (v == nullptr || center == nullptr || !IsValidCargoType(cargo) || quantity == 0 || v->owner != _current_company || center->owner != v->owner ||
		v->construction.placement != SpacePlacement::SurfaceStored || v->construction.home != GetLaunchCenterReference(*center) || (v->manifest && v->manifest->service_ticks != 0)) return CMD_ERROR;
	const auto model = GetSpaceVehicleModel(*v); const auto label = CargoSpec::Get(cargo)->label;
	// Surface freight belongs to rockets and the declared Steel/Goods constructor; occupied holds cannot be relabelled into another material owner.
	if (model != 14 && model != 20) return CMD_ERROR;
	if (model == 20 && label != CargoLabel{"STEL"} && label != CargoLabel{"GOOD"}) return CMD_ERROR;
	// Rocket freight uses only the launch supply contract; an otherwise valid label cannot bypass its physical handling rules.
	if (model == 14 && !IsLaunchSupplyCargo(cargo)) return CMD_ERROR;
	// An occupied hold keeps its original cargo identity so loading cannot relabel existing paid packets.
	if (v->cargo.TotalCount() != 0 && v->cargo_type != cargo) return CMD_ERROR;
	if (v->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) != v->cargo.TotalCount()) return CMD_ERROR;
	if (unload) {
		// Whole unloading requires real same-label cargo and complete station headroom; failed quotes retain every packet on the hull.
		if (v->cargo_type != cargo || quantity > GetLaunchStockHeadroom(*center, cargo) || !HubCargoTransfer::CanMove(v->cargo, quantity)) return CMD_ERROR;
	} else {
		const auto capacity = GetSpaceFreightCapacity(model, cargo);
		// All requested stock and role-specific hold capacity must exist before any packet changes ownership.
		if (v->cargo.TotalCount() > capacity || quantity > capacity - v->cargo.TotalCount() || !center->goods[cargo].HasData() || !HubCargoTransfer::CanMove(center->goods[cargo].GetData().cargo, quantity)) return CMD_ERROR;
		if (model == 14) {
			const auto mass = GetSpacePayloadMass(*v), extra = uint64_t{quantity} * CargoSpec::Get(cargo)->weight;
			// Quote the complete added physical mass against remaining rocket payload capacity before any cargo changes hands.
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
/** Return the same admitted original hull to its real Surface storage pose.
 * @param v Actual paid hull; no new body or inventory is produced. @param center Actual authorized same-owner center whose real zone supplies the tile.
 */
static void PutProbeInStorage(SpaceVehicle &v, Station &center)
{
	v.construction.placement = SpacePlacement::SurfaceStored; v.construction.paused = false;
	v.construction.surface_tile = GetLaunchFacilityZone(center.launch_center->tile, center.launch_center->rotation, LaunchFacilityZone::Storage)->tile;
	v.position = v.home = *GetLaunchCenterPlaneReference(center.launch_center->tile); v.UpdateSpacePosition(); SyncSpacePods(v); v.MarkDirty();
}
/** @copydoc SyncSpacePassenger */
void SyncSpacePassenger(SpaceVehicle &carrier)
{
	SyncSpacePods(carrier);
	if (!carrier.manifest) return;
	auto *v = ResolveSpaceCraft(carrier.manifest->passenger);
	// Only the reciprocal original passenger incarnation follows this carrier; a stale reference cannot move another paid hull.
	if (v == nullptr || !v->manifest || v->manifest->carrier != GetSpaceCraftReference(carrier)) return;
	v->construction.surface_tile = carrier.construction.surface_tile;
	v->position = carrier.position; v->home = carrier.home; v->UpdateSpacePosition(); v->MarkDirty();
	SyncSpacePods(*v);
}
/** @copydoc DetachSpaceManifestAtLostCenter */
void DetachSpaceManifestAtLostCenter(SpaceVehicle &v)
{
	if (!v.manifest) return;
	auto &m = *v.manifest;
	// Losing a home retires reciprocal carrier links while retaining the actual passenger and paused work; no fallback center grants storage.
	if (auto *carrier = ResolveSpaceCraft(m.carrier); carrier != nullptr && carrier->manifest && carrier->manifest->passenger == GetSpaceCraftReference(v)) carrier->manifest->passenger = {};
	if (auto *probe = ResolveSpaceCraft(m.passenger); probe != nullptr && probe->manifest && probe->manifest->carrier == GetSpaceCraftReference(v)) {
		probe->manifest->carrier = {}; probe->construction.home = {}; probe->construction.placement = SpacePlacement::Unhomed; probe->construction.paused = true;
		if (probe->manifest->service_ticks != 0) probe->manifest->service_paused = true;
		probe->MarkDirty();
	}
	m.carrier = {}; m.passenger = {}; if (m.service_ticks != 0) m.service_paused = true;
}
/** @copydoc ForgetSpaceManifest */
void ForgetSpaceManifest(SpaceVehicle &v, bool cleaning)
{
	if (!v.manifest) return;
	// Pool teardown already owns disposal; ordinary single-hull removal instead settles each real dependent asset below.
	if (cleaning) { for (auto &stock : v.manifest->supplies) stock.OnCleanPool(); return; }
	ForgetSpacePodCraft(v);
	// Clear only exact reciprocal passenger bindings so hull loss cannot detach an unrelated incarnation or manufacture replacement storage.
	if (auto *parent = ResolveSpaceCraft(v.manifest->carrier); parent != nullptr && parent->manifest && parent->manifest->passenger == GetSpaceCraftReference(v)) parent->manifest->passenger = {};
	if (auto *probe = ResolveSpaceCraft(v.manifest->passenger); probe != nullptr && probe->manifest && probe->manifest->carrier == GetSpaceCraftReference(v)) {
		probe->manifest->carrier = {};
		if (GetSpaceVehicleLayer(v) == MapLayer::Space) {
			probe->construction.placement = SpacePlacement::Orbital; probe->construction.paused = false; probe->construction.deployed = true;
			probe->vehstatus.Reset({VehState::Hidden, VehState::Stopped}); probe->UpdateSpacePosition(); SyncSpacePods(*probe); probe->MarkDirty(); return;
		}
		auto *center = ResolveLaunchCenter(probe->construction.home);
		// A surviving original owned center alone can receive the passenger; lost/foreign homes retain explicit unhomed custody.
		if (center != nullptr && center->owner == probe->owner) PutProbeInStorage(*probe, *center);
		else { probe->construction.home = {}; probe->construction.placement = SpacePlacement::Unhomed; probe->construction.paused = true; }
	}
}
/** Move actual fuel/service stock through the original owned Earth facility and native compartments.
 * @param flags Estimate or execute. @param station Actual facility ID. @param center_generation Exact facility incarnation.
 * @param vehicle Actual hull ID. @param generation Exact hull incarnation. @param index Explicit bounded physical supply compartment.
 * @param quantity Positive whole units. @param unload True returns actual held parts/fuel to stock; false loads available original station packets.
 * @return Complete physical headroom/custody admission or unchanged refusal, without service work or allowance renewal.
 */
CommandCost CmdMoveSpaceSupply(DoCommandFlags flags, StationID station, uint64_t center_generation, VehicleID vehicle, uint64_t generation, uint8_t index, uint16_t quantity, bool unload)
{
	auto *v = ResolveSpaceCraft({vehicle, generation}); auto *center = ResolveLaunchCenter({station, center_generation});
	// Physical supply handling requires one owned stored hull at its real original facility; active service keeps its compartments reserved.
	if (v == nullptr || center == nullptr || index >= SPACE_SUPPLY_COUNT || quantity == 0 || v->owner != _current_company || center->owner != v->owner ||
		v->construction.placement != SpacePlacement::SurfaceStored || v->construction.home != GetLaunchCenterReference(*center) || (v->manifest && v->manifest->service_ticks != 0)) return CMD_ERROR;
	const auto supply = static_cast<SpaceSupply>(index); const auto cargo = GetSpaceSupplyCargo(supply);
	if (!IsValidCargoType(cargo)) return CMD_ERROR;
	const uint held = v->manifest ? v->manifest->supplies[index].TotalCount() : 0;
	// Complete packet availability and destination headroom are quoted before either direction changes a physical owner.
	if (unload) {
		if (!v->manifest || quantity > GetLaunchStockHeadroom(*center, cargo) || !HubCargoTransfer::CanMove(v->manifest->supplies[index], quantity)) return CMD_ERROR;
	} else {
		const auto capacity = GetSpaceSupplyCapacity(GetSpaceVehicleModel(*v), supply);
		// The whole requested supply must fit its actual compartment and exist in unreserved station stock before transfer.
		if (held > capacity || quantity > capacity - held || !center->goods[cargo].HasData() || !HubCargoTransfer::CanMove(center->goods[cargo].GetData().cargo, quantity)) return CMD_ERROR;
		// Loose rocket service parts share payload mass; propellant remains in the separate bounded flight-fuel store.
		if (GetSpaceVehicleModel(*v) == 14 && supply != SpaceSupply::Propellant) {
			const auto mass = GetSpacePayloadMass(*v); const auto extra = uint64_t{quantity} * CargoSpec::Get(cargo)->weight;
			// Quote the complete added physical mass against remaining rocket payload capacity before any cargo changes hands.
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
/** Pack or unpack the same actual Earth-built probe, retaining one carrier and its original paid body/parts/shells.
 * @param flags Estimate or execute. @param rocket_id Original reusable carrier ID. @param rocket_generation Exact carrier incarnation.
 * @param probe_id Actual paid payload hull ID. @param probe_generation Exact payload incarnation. @param unload True returns it to real same-center storage.
 * @return Whole physical role/custody/mass admission or unchanged refusal; orbital-only factory products gain no Earth bootstrap.
 */
CommandCost CmdCarryLaunchProbe(DoCommandFlags flags, VehicleID rocket_id, uint64_t rocket_generation, VehicleID probe_id, uint64_t probe_generation, bool unload)
{
	auto *rocket = ResolveSpaceCraft({rocket_id, rocket_generation}); auto *probe = ResolveSpaceCraft({probe_id, probe_generation});
	// Packing requires distinct owned real Earth-capable hulls at their original common Surface facility; active service or flight retains custody.
	if (rocket == nullptr || probe == nullptr || rocket == probe || rocket->owner != _current_company || probe->owner != rocket->owner || GetSpaceVehicleModel(*rocket) != 14 ||
		(GetSpaceVehicleModel(*probe) != 15 && GetSpaceVehicleModel(*probe) != 19 && GetSpaceVehicleModel(*probe) != 20) ||
		rocket->construction.placement != SpacePlacement::SurfaceStored || rocket->construction.home != probe->construction.home ||
		(rocket->manifest && rocket->manifest->service_ticks != 0) || (probe->manifest && probe->manifest->service_ticks != 0)) return CMD_ERROR;
	// Only the original same-owner Surface home can authorize packing or unpacking; no guessed replacement center is used.
	auto *center = ResolveLaunchCenter(rocket->construction.home); if (center == nullptr || center->owner != rocket->owner) return CMD_ERROR;
	if (unload) {
		// Unpacking requires exact reciprocal original carrier references and actual carried placement before releasing the passenger.
		if (!rocket->manifest || !probe->manifest || rocket->manifest->passenger != GetSpaceCraftReference(*probe) || probe->manifest->carrier != GetSpaceCraftReference(*rocket) || probe->construction.placement != SpacePlacement::Carried) return CMD_ERROR;
	} else {
		// Packing consumes one available same-center stored passenger slot with no competing carrier or unfinished service.
		if (probe->construction.placement != SpacePlacement::SurfaceStored || (rocket->manifest && rocket->manifest->passenger != SpaceCraftRef{}) ||
			(probe->manifest && (probe->manifest->carrier != SpaceCraftRef{} || probe->manifest->passenger != SpaceCraftRef{}))) return CMD_ERROR;
		const auto own = GetSpacePayloadMass(*rocket), cargo = OwnPayloadMass(*probe);
		const auto dry = GetSpaceVehicleConstruction(GetSpaceVehicleModel(*probe))->dry_mass_sixteenths;
		// The paid passenger body and all existing payload must fit together; unknown mass refuses rather than creating weightless cargo.
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
 * @param flags Estimate or execute; mixed QueryCost preserves query precedence. @param vehicle Native probe ID. @param generation Exact hull incarnation.
 * @param cancel True pauses the existing service. @return Complete service admission/control or unchanged refusal.
 */
CommandCost CmdServiceLaunchProbe(DoCommandFlags flags, VehicleID vehicle, uint64_t generation, bool cancel)
{
	if (flags.Test(DoCommandFlag::QueryCost)) flags.Reset(DoCommandFlag::Execute);
	// A Core reservation is the sole service owner until completed or explicitly cancelled there.
	if (const auto *craft = ResolveSpaceCraft({vehicle, generation}); craft != nullptr && HasOrbitalServiceAssignment(*craft)) return CMD_ERROR;
	auto *v = ResolveSpaceCraft({vehicle, generation}); if (v == nullptr || v->owner != _current_company || !v->manifest ||
		(v->construction.placement != SpacePlacement::SurfaceStored && v->construction.placement != SpacePlacement::Orbital)) return CMD_ERROR;
	const auto model = GetSpaceVehicleModel(*v); const auto service = GetSpaceServiceTuning(model);
	// A service bill does not create placement rights: the appended factory-only heavy can use its actual orbital onboard owner, while existing Earth-capable roles retain their original workshop path.
	if (!service || (model == SPACE_HEAVY_MINER_MODEL && v->construction.placement != SpacePlacement::Orbital)) return CMD_ERROR;
	auto &m = *v->manifest;
	if (v->construction.placement == SpacePlacement::Orbital) {
		// Only an idle queue, held Mine or physically reached owned center Return/Service may use onboard work; active travel/scan/construction cannot borrow this allowance producer.
		if (m.pending_returns != 0 || v->mission.phase != SpaceMissionPhase::Holding ||
			(v->space_orders.cursor < v->space_orders.entries.size() && !IsSpaceMiningServiceWait(v->space_orders) && !IsSpaceOnboardServiceOrder(*v))) return CMD_ERROR;
		// Cancellation needs actual active onboard work; starting needs exhausted allowance and real unconsumed parts, preserving the once-only service bill.
		if (cancel ? m.service_ticks == 0 || m.service_paused || m.service_location != SpaceServiceLocation::Onboard :
			(m.service_ticks != 0 && !m.service_paused) || m.returns_remaining != 0 || m.services_completed == UINT64_MAX || m.supplies[1].AvailableCount() < service->inputs[0] || m.supplies[2].AvailableCount() < service->inputs[1]) return CMD_ERROR;
		if (flags.Test(DoCommandFlag::Execute)) {
			m.service_location = SpaceServiceLocation::Onboard; m.service_paused = cancel;
			// Only a fresh accepted start establishes the original countdown; resuming preserves already earned work.
			if (!cancel && m.service_ticks == 0) m.service_ticks = service->ticks;
			v->MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly);
		}
		return CommandCost{};
	}
	// Earth service keeps the actual owned original workshop and refuses while descent returns still reserve allowance.
	auto *center = ResolveLaunchCenter(v->construction.home); if (center == nullptr || center->owner != v->owner || !center->launch_operations || m.pending_returns != 0) return CMD_ERROR;
	auto &op = *center->launch_operations;
	// Workshop cancellation releases only this exact active job; another hull or a paused/nonexistent countdown cannot be canceled here.
	if (cancel) { if (m.service_ticks == 0 || m.service_paused || op.service != GetSpaceCraftReference(*v)) return CMD_ERROR; }
	// Fresh workshop service requires every shared work slot and actual parts, with no passenger/carrier conflict or overflowing service history.
	else if ((m.service_ticks != 0 && !m.service_paused) || m.services_completed == UINT64_MAX || op.workshop != SpaceCraftRef{} || op.service != SpaceCraftRef{} || op.pod_workshop != SpacePodRef{} ||
		m.supplies[1].AvailableCount() < service->inputs[0] || m.supplies[2].AvailableCount() < service->inputs[1] || m.carrier != SpaceCraftRef{} || m.passenger != SpaceCraftRef{}) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		if (cancel) { op.service = {}; m.service_paused = true; PutProbeInStorage(*v, *center); }
		else {
			op.service = GetSpaceCraftReference(*v); if (m.service_ticks == 0) m.service_ticks = service->ticks; m.service_paused = false; m.service_location = SpaceServiceLocation::EarthWorkshop;
			v->construction.surface_tile = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Workshop)->tile; v->UpdateSpacePosition(); SyncSpacePods(*v);
		}
		v->MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly);
	}
	return CommandCost{};
}
/** @copydoc IsSpaceServiceWorkTick */
bool IsSpaceServiceWorkTick(const SpaceVehicle &v)
{
	// Neither editor callbacks nor paused simulation can earn upkeep/work from an otherwise retained service countdown.
	if (_game_mode != GameMode::Normal || _pause_mode.Any()) return false;
	if (!v.manifest) return false;
	const auto &m = *v.manifest;
	const auto service = GetSpaceServiceTuning(GetSpaceVehicleModel(v));
	// Only active paid work with both real inputs earns a tick; missing stock or pause preserves earned progress and allowance.
	if (!service || m.service_ticks == 0 || m.service_ticks > service->ticks || m.service_paused || m.supplies[1].AvailableCount() < service->inputs[0] || m.supplies[2].AvailableCount() < service->inputs[1]) return false;
	if (m.service_location == SpaceServiceLocation::Onboard) {
		return v.construction.placement == SpacePlacement::Orbital && v.mission.phase == SpaceMissionPhase::Holding && m.pending_returns == 0;
	}
	const auto *center = ResolveLaunchCenter(v.construction.home);
	return v.construction.placement == SpacePlacement::SurfaceStored && center != nullptr && center->owner == v.owner && center->launch_operations &&
		center->launch_operations->service == GetSpaceCraftReference(v);
}

/** @copydoc GetSpaceResupplyBlock */
SpaceFlightBlock GetSpaceResupplyBlock(const SpaceVehicle &rocket, SpaceCraftRef receiver)
{
	const auto *v = ResolveSpaceCraft(receiver);
	// Direct resupply joins the original owned orbital receiver and rocket with no separate passenger promise.
	if (v == nullptr || v == &rocket || !Company::IsValidID(rocket.owner) || v->owner != rocket.owner || v->construction.placement != SpacePlacement::Orbital ||
		GetSpaceVehicleModel(rocket) != 14 || !rocket.manifest || rocket.manifest->passenger != SpaceCraftRef{}) return SpaceFlightBlock::TargetLost;
	const auto model = GetSpaceVehicleModel(*v);
	// The appended paid heavy miner receives actual shells/parts through the same complete original rocket payload transaction; constructor/hauler definitions gain no undeclared receiver role.
	if (model != 15 && model != 19 && model != 20 && model != SPACE_HEAVY_MINER_MODEL) return SpaceFlightBlock::TargetLost;
	// Incoming paid supplies retain their carrier while a Core owns the receiver's precise service berth.
	if (HasOrbitalServiceAssignment(*v) || v->mission.phase != SpaceMissionPhase::Holding || (v->manifest && v->manifest->service_ticks != 0) ||
	(v->space_orders.cursor < v->space_orders.entries.size() && !IsSpaceMiningServiceWait(v->space_orders) && !IsSpaceOnboardServiceOrder(*v))) return SpaceFlightBlock::ReceiverBusy;
	const auto &from = *rocket.manifest; const auto *to = v->manifest.get();
	const auto *miner = GetSpaceMiningTuning(model);
	const size_t pod_cap = miner != nullptr ? miner->starting_pods : GetMultiLayerSpaceBudget().starting_pods;
	const size_t held_pods = to == nullptr ? 0 : to->pods.size();
	// All original shells must fit together; refusing excess capacity retains the entire payload rather than delivering a privileged prefix.
	if (held_pods > pod_cap || from.pods.size() > pod_cap - held_pods) return SpaceFlightBlock::ReceiverFull;
	bool payload = !from.pods.empty() || rocket.cargo.TotalCount() != 0;
	// Every real parts compartment must fit in full and be unreserved before the once-only handover can proceed.
	for (uint i = 1; i < SPACE_SUPPLY_COUNT; ++i) {
		const auto count = from.supplies[i].TotalCount(); const auto capacity = GetSpaceSupplyCapacity(model, static_cast<SpaceSupply>(i));
		const auto held = to == nullptr ? 0U : to->supplies[i].TotalCount();
		// Every incoming part must fit in full and remain available; a full/reserved compartment retains the whole rocket payload.
		if (held > capacity || count > capacity - held || from.supplies[i].ReservedCount() != 0) return SpaceFlightBlock::ReceiverFull;
		payload |= count != 0;
	}
	if (!payload) return SpaceFlightBlock::ReceiverFull;
	// Verify exact exclusive shell custody before moving any parts or pod references; one missing/stale shell blocks the whole transaction.
	for (const auto ref : from.pods) {
		const auto *pod = ResolveSpacePod(ref);
		// Each shell must resolve to one real owned original carrier entry with no duplicate receiver claim before any delivery.
		if (pod == nullptr || pod->state.owner != rocket.owner || pod->state.custody != SpacePodCustody::Craft || pod->state.craft != GetSpaceCraftReference(rocket) ||
			std::count(from.pods.begin(), from.pods.end(), ref) != 1 || (to != nullptr && std::find(to->pods.begin(), to->pods.end(), ref) != to->pods.end())) return SpaceFlightBlock::TargetLost;
	}
	if (rocket.cargo.TotalCount() != 0) {
		if (!IsValidCargoType(rocket.cargo_type)) return SpaceFlightBlock::ReceiverFull;
		const auto cargo = rocket.cargo_type; const auto label = CargoSpec::Get(cargo)->label;
		const auto capacity = GetSpaceFreightCapacity(model, cargo);
		// General rocket freight remains constructor-only Steel/Goods; new miner supply rights grant no undeclared freight transport or relabelling.
		if (model != 20 || (label != CargoLabel{"STEL"} && label != CargoLabel{"GOOD"}) || (v->cargo.TotalCount() != 0 && v->cargo_type != cargo) ||
			v->cargo.TotalCount() > capacity || rocket.cargo.TotalCount() > capacity - v->cargo.TotalCount() ||
			rocket.cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) != rocket.cargo.TotalCount() ||
			v->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep) != v->cargo.TotalCount()) return SpaceFlightBlock::ReceiverFull;
	}
	return SpaceFlightBlock::None;
}

/** @copydoc CommitSpaceResupply */
bool CommitSpaceResupply(SpaceVehicle &rocket, SpaceCraftRef receiver)
{
	// Only this flight's original uncommitted delivery opportunity can hand over the completely admitted payload once.
	if (!rocket.transition || rocket.transition->phase != SpaceFlightPhase::OrbitalDelivery || rocket.transition->resupplied ||
		rocket.transition->destination != SpaceOrderTarget{SpaceOrderTargetKind::Craft, receiver.vehicle.base(), receiver.generation} ||
		GetSpaceResupplyBlock(rocket, receiver) != SpaceFlightBlock::None) return false;
	auto *v = ResolveSpaceCraft(receiver);
	if (SpaceDistanceSquared(rocket.position, v->position) > uint64_t{SPACE_Q16 / 4} * (SPACE_Q16 / 4)) return false;
	auto &from = *rocket.manifest; auto &to = GetOrCreateSpaceManifest(*v); // Old absence means zero stock/quota; delivered paid inputs create only this native compartment owner.
	to.pods.reserve(to.pods.size() + from.pods.size()); // Allocate vector headroom before physical owner changes.
	// Transfer complete original compartment lists before shell custody; the prior whole preflight guarantees no partial packet allocation is needed.
	for (uint i = 1; i < SPACE_SUPPLY_COUNT; ++i) if (const auto count = from.supplies[i].TotalCount(); count != 0) {
		const bool moved = HubCargoTransfer::Move(from.supplies[i], to.supplies[i], count); assert(moved); (void)moved; // Whole lists require no packet splits.
	}
	if (const auto count = rocket.cargo.TotalCount(); count != 0) {
		v->cargo_type = rocket.cargo_type; v->cargo_cap = GetSpaceFreightCapacity(GetSpaceVehicleModel(*v), v->cargo_type);
		const auto moved = rocket.cargo.Shift(count, &v->cargo); assert(moved == count); (void)moved;
	}
	// The same paid shells change their sole carrier, then follow the receiver's actual pose while retaining valid Surface history.
	for (const auto ref : from.pods) { ResolveSpacePod(ref)->state.craft = receiver; to.pods.push_back(ref); }
	from.pods.clear(); SyncSpacePods(*v); v->MarkDirty(); rocket.MarkDirty(); SetWindowClassesDirty(WindowClass::LaunchAssembly);
	return true;
}

/** @copydoc TickSpaceManifests */
void TickSpaceManifests()
{
	// Only a normal unpaused native opportunity ages physical compartments or earns service work; direct repeated paused/editor callbacks cannot substitute simulation time.
	if (_game_mode != GameMode::Normal || _pause_mode.Any()) return;
	// Age each original physical compartment once and advance only its already admitted service owner; resupply alone starts no work.
	for (auto *v : SpaceVehicle::Iterate()) if (v->manifest) {
		auto &m = *v->manifest;
		if (++m.age_ticks == Ticks::CARGO_AGING_TICKS) { m.age_ticks = 0; for (auto &stock : m.supplies) HubCargoTransfer::Age(stock); }
		// Supply aging does not start service; only an existing unpaused paid countdown may advance.
		if (m.service_ticks == 0 || m.service_paused) continue;
		if (!IsSpaceServiceWorkTick(*v)) { m.service_paused = true; continue; }
		Station *center = m.service_location == SpaceServiceLocation::EarthWorkshop ? ResolveLaunchCenter(v->construction.home) : nullptr;
		if (--m.service_ticks == 0) {
			const auto service = GetSpaceServiceTuning(GetSpaceVehicleModel(*v)); assert(service);
			// This same native opportunity preflighted both complete actual inputs. Consume the immutable role bill once and replace, rather than add, allowance only after the last earned tick.
			const bool metal = HubCargoTransfer::Consume(m.supplies[1], service->inputs[0]), electronics = HubCargoTransfer::Consume(m.supplies[2], service->inputs[1]); assert(metal && electronics); (void)metal; (void)electronics;
			++m.services_completed; m.returns_remaining = service->returns; m.service_location = SpaceServiceLocation::EarthWorkshop;
			if (center != nullptr) { center->launch_operations->service = {}; PutProbeInStorage(*v, *center); }
			SetWindowClassesDirty(WindowClass::LaunchAssembly);
		}
	}
}

/** @copydoc CaptureSpaceManifests */
std::vector<SpaceManifestRecord> CaptureSpaceManifests()
{
	std::vector<SpaceManifestRecord> result;
	// Preserve each actual native owner's complete references/history in stable order; detached records never acquire their own packet ownership.
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
/** @copydoc ValidateSpaceManifestRecords */
bool ValidateSpaceManifestRecords(std::span<const SpaceManifestRecord> records, bool heavy_onboard)
{
	if (records.size() > MAX_SPACE_VEHICLES) return false;
	const auto service_limit = heavy_onboard ? GetSpaceServiceTuning(SPACE_HEAVY_MINER_MODEL)->ticks : GetMultiLayerSpaceBudget().service_ticks;
	std::set<VehicleID> craft; std::set<uint64_t> generations; std::set<CargoPacketID> packets; std::set<SpacePodID> pods;
	/** @param r Optional copied native hull reference. @return Whether absence or a structurally meaningful issued incarnation can be validated by its actual owner. */
	auto ref = [](SpaceCraftRef r) { return r == SpaceCraftRef{} || (r.vehicle != VehicleID::Invalid() && r.generation != 0); };
	// Every manifest must retain one exclusive incarnation, bounded service/return history and unique physical references before same-file/native role joins.
	for (const auto &r : records) {
		if (r.craft == SpaceCraftRef{} || !ref(r.craft) || !ref(r.passenger) || !ref(r.carrier) || !craft.insert(r.craft.vehicle).second || !generations.insert(r.craft.generation).second ||
			r.passenger == r.craft || r.carrier == r.craft || (r.passenger != SpaceCraftRef{} && r.carrier != SpaceCraftRef{}) ||
			r.age_ticks >= Ticks::CARGO_AGING_TICKS || r.service_ticks > service_limit ||
			r.returns_remaining > GetMultiLayerSpaceBudget().returns_per_service || r.pending_returns > r.returns_remaining || r.service_location >= SpaceServiceLocation::End ||
			(r.service_ticks == 0 && (r.service_paused || r.service_location != SpaceServiceLocation::EarthWorkshop)) ||
			(r.service_ticks != 0 && r.pending_returns != 0) ||
			(r.service_ticks != 0 && r.services_completed == UINT64_MAX)) return false;
		if (r.pods.size() > GetMultiLayerSpaceBudget().launch_pod_cap) return false;
		// A shell or packet appears in exactly one compartment across the whole family; duplicated references cannot create capacity or a second return.
		for (const auto ref : r.pods) if (ref.pod >= SpacePodID::End() || ref.generation == 0 || !pods.insert(ref.pod).second) return false;
		for (const auto &slot : r.supplies) {
			if (slot.size() > 128) return false;
			// Each physical packet keeps one exclusive manifest owner and a bounded routing reference; duplicate stock cannot finance multiple services.
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
	/** @param list Existing physical packet container. @return Whether every actual packet is exclusive across all earlier authoritative containers. */
	auto add = [&](const auto &list) { for (const auto *cp : list) if (!owned.insert(cp->index).second) return false; return true; };
	// Include ordinary station stock, Surface handling and all vehicle holds, since no orbital compartment may also own those physical packets.
	for (const auto *station : Station::Iterate()) {
		for (CargoType cargo : EnumRange(NUM_CARGO)) if (station->goods[cargo].HasData()) {
			for (const auto &[hop, packets] : *station->goods[cargo].GetData().cargo.Packets()) { (void)hop; if (!add(packets)) return false; }
		}
		if (station->launch_hub) for (const auto &job : station->launch_hub->handling) for (const auto &[hop, packets] : *job->packets.Packets()) { (void)hop; if (!add(packets)) return false; }
	}
	for (const auto *v : Vehicle::Iterate()) if (!add(*v->cargo.Packets())) return false;
	return true;
}
/** @param records Complete copied native manifests. @return Whether every declared role, physical packet owner and custody relationship agrees with the actual native fleet. */
static bool ManifestPackets(std::span<const SpaceManifestRecord> records)
{
	std::set<CargoPacketID> owned; if (!OtherPacketOwners(owned)) return false;
	// Join every copied record to its actual role and physical packets; broad structural caps alone cannot authorize freight, fuel or service.
	for (const auto &r : records) {
		const auto *v = ResolveSpaceCraft(r.craft); if (v == nullptr || !Company::IsValidID(v->owner)) return false;
		const auto model = GetSpaceVehicleModel(*v);
		// The native current-world invariant also rejects heavy work/history whose producer is still unavailable; save queries and direct installation cannot bypass the separate full/copy source-era gate.
		if (!IsSpaceHeavyResupplyStateSupported(model, r, true, true, true)) return false;
		const auto service = GetSpaceServiceTuning(model);
		// Broad source transport bounds cannot lend a legacy or unrelated role the heavy's larger service budget or completed history.
		if ((r.service_ticks != 0 || r.services_completed != 0) && (!service || r.service_ticks > service->ticks)) return false;
		// A manifest belongs only to a delivered canonical native role; the new orbital products retain the same sole packet/service owner as existing hulls.
		if (model != 14 && model != 15 && model != 16 && model != 17 && model != 19 && model != 20 && model != SPACE_HEAVY_MINER_MODEL) return false;
		if (model == 14 && (r.carrier != SpaceCraftRef{} || r.service_ticks != 0 || r.returns_remaining != 0 || r.pending_returns != 0 || r.services_completed != 0)) return false;
		// A non-rocket role cannot own passenger deployment or rocket fuel history, even when its compartment references are structurally valid.
		if (model != 14 && (r.passenger != SpaceCraftRef{} || r.fuel_burned != 0)) return false;
		// Resolve whole original packet lists and their role-specific capacities, including the real parts needed by any retained service work.
		for (uint i = 0; i < SPACE_SUPPLY_COUNT; ++i) {
			uint64_t count = 0;
			for (const auto &[id, hop] : r.supplies[i]) {
				(void)hop; const auto *cp = CargoPacket::GetIfValid(id);
				// Every original packet must exist and remain exclusive across all native inventory owners before its quantity counts toward capacity.
				if (cp == nullptr || !owned.insert(id).second) return false; count += cp->Count();
			}
			if (count > GetSpaceSupplyCapacity(model, static_cast<SpaceSupply>(i))) return false;
			// Retained original service work requires the actual unconsumed part in each service compartment; a countdown alone grants no completion.
			if (r.service_ticks != 0 && i != 0 && count < service->inputs[i - 1]) return false;
		}
		/** @param craft Exact copied hull incarnation. @return Its same-file original manifest; a reused numeric hull cannot substitute custody. */
		auto find = [&](SpaceCraftRef craft) -> const SpaceManifestRecord * { for (const auto &candidate : records) if (candidate.craft == craft) return &candidate; return nullptr; };
		if (r.passenger != SpaceCraftRef{}) {
			const auto *child = find(r.passenger); const auto *probe = ResolveSpaceCraft(r.passenger);
			// A passenger is the exact same-owner original hull with reciprocal custody, never a second rocket or an unrelated numeric pool ID.
			if (child == nullptr || probe == nullptr || child->carrier != r.craft || probe->owner != v->owner || GetSpaceVehicleModel(*probe) == 14) return false;
		}
		if (r.carrier != SpaceCraftRef{}) {
			const auto *parent = find(r.carrier); const auto *rocket = ResolveSpaceCraft(r.carrier);
			// Carried placement resolves only through the original same-owner rocket and its matching reciprocal passenger claim.
			if (parent == nullptr || rocket == nullptr || parent->passenger != r.craft || rocket->owner != v->owner || GetSpaceVehicleModel(*rocket) != 14) return false;
		}
	}
	return true;
}
/** @copydoc InstallLoadedSpaceManifests */
bool InstallLoadedSpaceManifests(std::span<const SpaceManifestRecord> records)
{
	// Structural bounds and exclusive actual packet custody must both pass before native work, installation or pose checks use these records.
	if (!ValidateSpaceManifestRecords(records, true) || !ManifestPackets(records)) return false;
	// Fresh reconstruction attaches the staged sole owners only after whole-family preflight; replacing a live manifest could destroy paid inventory.
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
/** Validate the actual native passenger, packet, paid-service and current instruction owners together; a reached service order never supplies its own allowance or parts.
 * @return Whether every installed manifest agrees with its real hull, physical producer and service-compatible queue.
 */
bool ValidateSpaceManifests()
{
	const auto records = CaptureSpaceManifests();
	// Structural and physical exclusivity precede live pose/work checks, so reciprocal parent lookups below refer to actual admitted native owners.
	// Structural bounds and exclusive actual packet custody must both pass before native work, installation or pose checks use these records.
	if (!ValidateSpaceManifestRecords(records, true) || !ManifestPackets(records)) return false;
	// Enforce paid allowance provenance, complete carried poses and the real current service slot for every native manifest.
	for (const auto *v : SpaceVehicle::Iterate()) {
		if (!v->manifest) { if (v->construction.placement == SpacePlacement::Carried) return false; continue; }
		const auto &m = *v->manifest;
		// Legacy unpaid placement cannot acquire a fresh initial allowance; only real completed service can establish later paid returns.
		if (v->construction.content_version == 0 && m.returns_remaining != 0 && m.services_completed == 0) return false;
		for (const auto &slot : m.supplies) if (slot.ReservedCount() != 0) return false;
		// Installed rocket custody remains subject to the complete physical payload limit after reconstruction.
		if (GetSpaceVehicleModel(*v) == 14 && GetSpacePayloadMass(*v) > GetMultiLayerSpaceBudget().rocket_payload_sixteenths) return false;
		if (m.carrier != SpaceCraftRef{}) {
			const auto *parent = ResolveSpaceCraft(m.carrier);
			// The passenger follows its actual admitted original carrier in every native coordinate/home field; inconsistent partial poses cannot authorize flight.
			if (v->construction.placement != SpacePlacement::Carried || (parent->construction.placement != SpacePlacement::SurfaceStored && parent->construction.placement != SpacePlacement::InTransit) ||
				v->construction.home != parent->construction.home || v->tile != parent->tile || v->x_pos != parent->x_pos || v->y_pos != parent->y_pos || v->z_pos != parent->z_pos ||
				v->position != parent->position || v->home != parent->home) return false;
		} else if (v->construction.placement == SpacePlacement::Carried) return false;
		if (m.service_ticks != 0) {
			if (m.service_location == SpaceServiceLocation::Onboard) {
				// Onboard work requires a genuinely held orbital hull with exhausted allowance and a service-compatible queue, never simultaneous travel or extraction.
				if (v->construction.placement != SpacePlacement::Orbital || v->mission.phase != SpaceMissionPhase::Holding || m.returns_remaining != 0 ||
					(v->space_orders.cursor < v->space_orders.entries.size() && !IsSpaceMiningServiceWait(v->space_orders) && !IsSpaceOnboardServiceOrder(*v))) return false;
			} else if (v->construction.placement == SpacePlacement::Unhomed) { if (!m.service_paused) return false; }
			else {
				const auto *center = ResolveLaunchCenter(v->construction.home);
				// Earth work retains the exact original owned workshop; an unpaused countdown must also own that facility service slot.
				if (v->construction.placement != SpacePlacement::SurfaceStored || center == nullptr || center->owner != v->owner || !center->launch_operations ||
					(!m.service_paused && center->launch_operations->service != GetSpaceCraftReference(*v))) return false;
			}
		}
	}
	return true;
}
/** @copydoc ComputeSpaceManifestDigest */
uint64_t ComputeSpaceManifestDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Original scalar, encoded in fixed byte order so physical owner comparisons do not depend on host layout. */
	auto add = [&](uint64_t value) { for (uint i = 0; i < 8; ++i) { hash ^= (value >> (i * 8)) & 255; hash *= 1099511628211ULL; } };
	/** @param r Exact original hull incarnation; generation distinguishes a retired owner from a reused native pool ID. */
	auto ref = [&](SpaceCraftRef r) { add(r.vehicle.base()); add(r.generation); };
	// Hash the complete physical asset/history graph, including each actual packet's quantity and income provenance, for exact conservation checks.
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
