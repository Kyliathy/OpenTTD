/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_production.cpp Native Earth/orbital paid receipt and placement validation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group8B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups8/8BC; replacement shares native freight/placement restrictions.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12; paid haulers retain original processed/imported packet provenance.
 */
#include "stdafx.h"
#include "space_mining_tuning.h"
#include "space_production.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_transition.h"
#include "space_replacement.h"
#include "space_structure.h"
#include "launch_operations.h"
#include "multilayer_space_content.h"
#include "station_base.h"
#include "company_base.h"
#include "map_func.h"
#include "landscape.h"
#include <set>
#include "safeguards.h"

/** @copydoc GetSpaceCraftReference */
SpaceCraftRef GetSpaceCraftReference(const SpaceVehicle &v) { return {v.index, v.generation}; }
/** @copydoc ResolveSpaceCraft */
SpaceVehicle *ResolveSpaceCraft(SpaceCraftRef ref)
{
	auto *v = SpaceVehicle::GetIfValid(ref.vehicle);
	return v != nullptr && ref.generation != 0 && v->generation == ref.generation ? v : nullptr;
}
/** @copydoc CaptureSpaceProduction */
std::vector<SpaceProductionRecord> CaptureSpaceProduction()
{
	std::vector<SpaceProductionRecord> records;
	for (const auto *v : SpaceVehicle::Iterate()) records.push_back({GetSpaceCraftReference(*v), v->construction});
	return records;
}
/** @param ref Original Earth launch incarnation. @return Whether its native identity is structurally valid. */
static bool ValidCenterRef(LaunchCenterRef ref) { return ref.station < StationID::End() && ref.generation != 0; }

/** @copydoc ValidateSpaceProductionRecords */
bool ValidateSpaceProductionRecords(std::span<const SpaceProductionRecord> records, uint32_t width, uint32_t height)
{
	// Only bounded native fleets on supported world planes can carry commissioning receipts.
	if (records.size() > MAX_SPACE_VEHICLES || width < 64 || height < 64 || width > 4096 || height > 4096) return false;
	std::set<VehicleID> ids; std::set<uint64_t> generations;
	// Validate the whole copied fleet before publication; every paid hull must retain its original Earth or orbital factory receipt exactly once.
	for (const auto &record : records) {
		const auto &state = record.state;
		if (record.craft.vehicle == VehicleID::Invalid() || record.craft.generation == 0 || !ids.insert(record.craft.vehicle).second ||
			!generations.insert(record.craft.generation).second || state.placement >= SpacePlacement::End) return false;
		if (state.content_version == 0) { if (state != SpaceConstructionState{}) return false; continue; }
		const auto *definition = GetSpaceVehicleConstruction(state.model);
		// Both physical producers retain exact paid2 receipts after content3 arrives; a new21 role must instead prove its own3 recipe, capital and bounded work.
		if (!IsSupportedSpaceVehicleContentVersion(state.content_version, state.model) || definition == nullptr || state.capital_paid != definition->capital ||
			state.remaining_ticks > definition->work_ticks) return false;
		if (state.assembly != SpaceStructureRef{}) {
			// Orbital products are already complete/deployed and name an original factory/job; no fictitious Surface tile, home or launch origin may authorize them.
			if (state.assembly.structure >= SpaceStructureID::End() || state.assembly.generation == 0 || state.assembly_serial == 0 ||
				!CanCommissionSpaceVehicle(state.model, MultiLayerCommissioningFacility::OrbitalAssembly, true) ||
				state.home != LaunchCenterRef{} || state.origin != LaunchCenterRef{} || state.surface_tile != INVALID_TILE ||
				state.placement != SpacePlacement::Orbital || state.remaining_ticks != 0 || state.paused || !state.deployed) return false;
			continue;
		}
		// Existing Earth commissioning retains its actual launch identity, Surface geometry and transport phases; later heavy models cannot borrow that producer.
		if (state.assembly_serial != 0 || (state.model != 14 && state.model != 15 && state.model != 19 && state.model != 20) ||
			!ValidCenterRef(state.origin) || state.surface_tile.base() >= uint64_t{width} * height) return false;
		if (state.placement == SpacePlacement::Unhomed) { if (state.home != LaunchCenterRef{} || !state.paused) return false; }
		else if (state.home == LaunchCenterRef{}) { if (state.placement != SpacePlacement::Orbital && state.placement != SpacePlacement::InTransit && state.placement != SpacePlacement::Carried) return false; }
		else if (!ValidCenterRef(state.home)) return false;
		// Carried/flight/storage and deployed orbital states have distinct native movement owners; stopped incomplete work cannot masquerade as a delivered hull.
		if (((state.placement == SpacePlacement::SurfaceStored || state.placement == SpacePlacement::Carried || state.placement == SpacePlacement::InTransit) && state.paused) ||
			(state.placement == SpacePlacement::Orbital && (state.model == 14 || state.paused || !state.deployed)) ||
			(state.placement != SpacePlacement::Orbital && state.deployed) || (state.placement == SpacePlacement::InTransit && state.model != 14) ||
			(state.placement == SpacePlacement::Assembling && state.remaining_ticks == 0) ||
			((state.placement == SpacePlacement::SurfaceStored || state.placement == SpacePlacement::Carried || state.placement == SpacePlacement::Orbital || state.placement == SpacePlacement::InTransit) && state.remaining_ticks != 0)) return false;
	}
	return true;
}

/** @copydoc CanRetainSpaceFreight */
bool CanRetainSpaceFreight(uint16_t model, SpacePlacement placement, CargoType cargo, uint quantity)
{
	const auto capacity = GetSpaceFreightCapacity(model, cargo);
	// Every whole holding needs a real physical label and sufficient immutable-role mass capacity. Haulers also retain a declared refit while empty; zero stock cannot authorize an otherwise unsupported cargo or disagree with independent copied admission.
	if (!IsValidCargoType(cargo) || quantity > capacity || (model == 17 && capacity == 0)) return false;
	if (quantity == 0) return true;
	// Construction-probe freight remains the two construction labels accepted by its actual native loading/receipt owners, even when another role previously held the packets.
	if (model == 20 && CargoSpec::Get(cargo)->label != CargoLabel{"STEL"} && CargoSpec::Get(cargo)->label != CargoLabel{"GOOD"}) return false;
	// Incomplete assembly and dormant mining/survey probes cannot own Surface freight; active orbital holding and the original rocket/construction transport roles retain their existing rules.
	return placement != SpacePlacement::Assembling && (placement == SpacePlacement::Orbital || (GetSpaceMiningTuning(model) == nullptr && model != 19));
}

/** @copydoc CanRetainSpaceCargoPacket */
bool CanRetainSpaceCargoPacket(const SpaceConstructionState &state, const CargoPacket &packet)
{
	// Paid constructors and cargo haulers transport actual imported or processed stock without relabelling it as asteroid extraction. Other orbital roles retain their original mining provenance restriction; separate construction validation still proves each paid hull.
	return state.placement != SpacePlacement::Orbital || packet.GetSource().type == SourceType::Asteroid ||
		((state.model == 20 || state.model == 17) && state.content_version != 0);
}

/** @param records Complete admitted copied fleet. @param compare_live Whether installed receipt equality is required. @return Whether all native hull, cargo and physical producer owners agree. */
static bool Physical(std::span<const SpaceProductionRecord> records, bool compare_live)
{
	if (records.size() != GetSpaceVehicleCount() || (!records.empty() && GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry)) return false;
	for (const auto &record : records) {
		const auto *v = ResolveSpaceCraft(record.craft); const auto &s = record.state;
		if (v == nullptr || !Company::IsValidID(v->owner) || (compare_live && v->construction != s)) return false;
		const auto model = GetSpaceVehicleModel(*v);
		if (s.content_version == 0) { if (model != 15) return false; continue; }
		for (const auto point : {v->position, v->home}) if (point.x < 0 || point.y < 0 || point.x > int64_t{Map::MaxX()} * SPACE_Q16 || point.y > int64_t{Map::MaxY()} * SPACE_Q16) return false;
		// Receipt validation and replacement use the same whole-hold policy; a valid mass alone cannot grant another role's freight or dormant Surface custody.
		if (model != s.model || s.origin.generation >= GetNextLaunchCenterGeneration() ||
			v->cargo_cap != GetSpaceFreightCapacity(model, v->cargo_type) || !CanRetainSpaceFreight(model, s.placement, v->cargo_type, v->cargo.TotalCount())) return false;
		if (s.placement == SpacePlacement::Orbital) continue; // Native orbital mission/position validation runs over the installed state below.
		// The original paid in-transit rocket retains actual freight for either explicit craft resupply or direct module delivery; neither creates an orbital replacement hull.
		const bool freight_flight = model == 14 && IsSpaceFlightActive(*v) &&
			(v->transition->destination.kind == SpaceOrderTargetKind::Craft || v->transition->destination.kind == SpaceOrderTargetKind::Structure);
		const bool closing = freight_flight && v->transition->phase == SpaceFlightPhase::OrbitalDelivery;
		if (!v->space_orders.entries.empty() || v->mission.phase != SpaceMissionPhase::Holding ||
			v->mission.target != INVALID_ORBITAL_BODY || v->mission.work_ticks != 0 || v->mission.approach_ticks_left != 0 ||
			v->mission.travel_remainder >= 74 || (v->mission.travel_remainder != 0 && !closing) || v->mission.cost_remainder != 0 || v->extracted_units != 0 ||
			!v->vehstatus.Test(VehState::Stopped) || (s.placement != SpacePlacement::InTransit && !v->vehstatus.Test(VehState::Hidden)) ||
			(s.placement == SpacePlacement::InTransit && !freight_flight && v->cargo.TotalCount() != 0)) return false;
		MapLayer layer = MapLayer::Surface; bool retained_z = s.placement == SpacePlacement::Unhomed;
		if (s.placement == SpacePlacement::InTransit) {
			if (!IsSpaceFlightActive(*v)) return false; layer = v->transition->layer;
			retained_z = s.home == LaunchCenterRef{} && v->transition->phase == SpaceFlightPhase::Holding && layer == MapLayer::Surface;
		} else if (s.placement == SpacePlacement::Carried) {
			if (!v->manifest) return false; const auto *parent = ResolveSpaceCraft(v->manifest->carrier); if (parent == nullptr) return false;
			const auto state = std::find_if(records.begin(), records.end(), [&](const auto &r) { return r.craft == GetSpaceCraftReference(*parent); });
			if (state == records.end()) return false;
			if (state->state.placement == SpacePlacement::InTransit) { if (!IsSpaceFlightActive(*parent)) return false; layer = parent->transition->layer; }
			retained_z = state->state.home == LaunchCenterRef{} && parent->transition && parent->transition->phase == SpaceFlightPhase::Holding && layer == MapLayer::Surface;
		}
		if (layer == MapLayer::Space) {
			const int x = static_cast<int64_t>(v->position.x) * TILE_SIZE / SPACE_Q16 + TILE_SIZE / 2;
			const int y = static_cast<int64_t>(v->position.y) * TILE_SIZE / SPACE_Q16 + TileY(TileXYL(0, 0, MapLayer::Space)) * TILE_SIZE + TILE_SIZE / 2;
			if (v->x_pos != x || v->y_pos != y || v->z_pos != 0 || v->tile != TileVirtXY(x, y)) return false;
			continue;
		}
		if (v->tile != s.surface_tile ||
			v->x_pos != static_cast<int>(TileX(s.surface_tile) * TILE_SIZE + TILE_SIZE / 2) ||
			v->y_pos != static_cast<int>(TileY(s.surface_tile) * TILE_SIZE + TILE_SIZE / 2) ||
			(!retained_z && v->z_pos != GetTilePixelZ(s.surface_tile))) return false;
		if (s.placement != SpacePlacement::Unhomed && s.home != LaunchCenterRef{}) {
			const auto *center = ResolveLaunchCenter(s.home);
			if (center == nullptr || center->owner != v->owner) return false;
			const auto area = GetLaunchCenterFootprint(center->launch_center->tile, center->launch_center->rotation);
			if (!area || !area->Contains(s.surface_tile)) return false;
			if (v->position != *GetLaunchCenterPlaneReference(center->launch_center->tile) || v->home != v->position) return false;
		}
	}
	return true;
}
/** @copydoc ValidateSpaceProduction */
bool ValidateSpaceProduction()
{
	const auto records = CaptureSpaceProduction();
	// Complete-world save validation includes both actual replacement producers. Partial load installation below deliberately waits for detached same-file structure validation.
	return ValidateSpaceProductionRecords(records, Map::SizeX(), Map::SizeY()) && Physical(records, true) &&
		ValidateSpaceReplacementLinks(CaptureSpaceStructures(), records, GetNextSpaceVehicleGeneration());
}
/** @copydoc InstallLoadedSpaceProduction */
bool InstallLoadedSpaceProduction(std::span<const SpaceProductionRecord> records)
{
	if (!ValidateSpaceProductionRecords(records, Map::SizeX(), Map::SizeY()) || !Physical(records, false)) return false;
	for (const auto &record : records) ResolveSpaceCraft(record.craft)->construction = record.state;
	return true;
}
