/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_production.cpp Native paid receipt/placement validation; Plan9 Group8B. */
#include "stdafx.h"
#include "space_production.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_transition.h"
#include "launch_operations.h"
#include "multilayer_space_content.h"
#include "station_base.h"
#include "company_base.h"
#include "map_func.h"
#include "landscape.h"
#include <set>
#include "safeguards.h"

SpaceCraftRef GetSpaceCraftReference(const SpaceVehicle &v) { return {v.index, v.generation}; }
SpaceVehicle *ResolveSpaceCraft(SpaceCraftRef ref)
{
	auto *v = SpaceVehicle::GetIfValid(ref.vehicle);
	return v != nullptr && ref.generation != 0 && v->generation == ref.generation ? v : nullptr;
}
std::vector<SpaceProductionRecord> CaptureSpaceProduction()
{
	std::vector<SpaceProductionRecord> records;
	for (const auto *v : SpaceVehicle::Iterate()) records.push_back({GetSpaceCraftReference(*v), v->construction});
	return records;
}
static bool ValidCenterRef(LaunchCenterRef ref) { return ref.station < StationID::End() && ref.generation != 0; }

bool ValidateSpaceProductionRecords(std::span<const SpaceProductionRecord> records, uint32_t width, uint32_t height)
{
	if (records.size() > MAX_SPACE_VEHICLES || width < 64 || height < 64 || width > 4096 || height > 4096) return false;
	std::set<VehicleID> ids; std::set<uint64_t> generations;
	for (const auto &record : records) {
		const auto &s = record.state;
		if (record.craft.vehicle == VehicleID::Invalid() || record.craft.generation == 0 || !ids.insert(record.craft.vehicle).second ||
			!generations.insert(record.craft.generation).second || s.placement >= SpacePlacement::End) return false;
		if (s.content_version == 0) { if (s != SpaceConstructionState{}) return false; continue; }
		const auto *d = GetSpaceVehicleConstruction(s.model);
		if (s.content_version != MULTILAYER_SPACE_CONTENT_VERSION || d == nullptr || (s.model != 14 && s.model != 15 && s.model != 19 && s.model != 20) ||
			s.capital_paid != d->capital || !ValidCenterRef(s.origin) || s.surface_tile.base() >= uint64_t{width} * height ||
			s.remaining_ticks > d->work_ticks) return false;
		if (s.placement == SpacePlacement::Unhomed) { if (s.home != LaunchCenterRef{} || !s.paused) return false; }
		else if (s.home == LaunchCenterRef{}) { if (s.placement != SpacePlacement::Orbital && s.placement != SpacePlacement::InTransit && s.placement != SpacePlacement::Carried) return false; }
		else if (!ValidCenterRef(s.home)) return false;
		if ((s.placement == SpacePlacement::SurfaceStored || s.placement == SpacePlacement::Carried || s.placement == SpacePlacement::InTransit) && s.paused) return false;
		if (s.placement == SpacePlacement::Orbital && (s.model == 14 || s.paused || !s.deployed)) return false;
		if (s.placement != SpacePlacement::Orbital && s.deployed) return false;
		if (s.placement == SpacePlacement::InTransit && s.model != 14) return false;
		if ((s.placement == SpacePlacement::Assembling && s.remaining_ticks == 0) ||
			((s.placement == SpacePlacement::SurfaceStored || s.placement == SpacePlacement::Carried || s.placement == SpacePlacement::Orbital || s.placement == SpacePlacement::InTransit) && s.remaining_ticks != 0)) return false;
	}
	return true;
}

static bool Physical(std::span<const SpaceProductionRecord> records, bool compare_live)
{
	if (records.size() != GetSpaceVehicleCount() || (!records.empty() && GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry)) return false;
	for (const auto &record : records) {
		const auto *v = ResolveSpaceCraft(record.craft); const auto &s = record.state;
		if (v == nullptr || !Company::IsValidID(v->owner) || (compare_live && v->construction != s)) return false;
		const auto model = GetSpaceVehicleModel(*v);
		if (s.content_version == 0) { if (model != 15) return false; continue; }
		for (const auto point : {v->position, v->home}) if (point.x < 0 || point.y < 0 || point.x > int64_t{Map::MaxX()} * SPACE_Q16 || point.y > int64_t{Map::MaxY()} * SPACE_Q16) return false;
		if (model != s.model || s.origin.generation >= GetNextLaunchCenterGeneration() ||
			v->cargo_cap != GetSpaceFreightCapacity(model, v->cargo_type) || v->cargo.TotalCount() > v->cargo_cap ||
			(v->cargo.TotalCount() != 0 && model == 20 && CargoSpec::Get(v->cargo_type)->label != CargoLabel{"STEL"} && CargoSpec::Get(v->cargo_type)->label != CargoLabel{"GOOD"})) return false;
		if (s.placement == SpacePlacement::Orbital) continue; // Native orbital mission/position validation runs over the installed state below.
		// The original paid in-transit rocket retains actual freight for either explicit craft resupply or direct module delivery; neither creates an orbital replacement hull.
		const bool freight_flight = model == 14 && IsSpaceFlightActive(*v) &&
			(v->transition->destination.kind == SpaceOrderTargetKind::Craft || v->transition->destination.kind == SpaceOrderTargetKind::Structure);
		const bool closing = freight_flight && v->transition->phase == SpaceFlightPhase::OrbitalDelivery;
		if (!v->space_orders.entries.empty() || v->mission.phase != SpaceMissionPhase::Holding ||
			v->mission.target != INVALID_ORBITAL_BODY || v->mission.work_ticks != 0 || v->mission.approach_ticks_left != 0 ||
			v->mission.travel_remainder >= 74 || (v->mission.travel_remainder != 0 && !closing) || v->mission.cost_remainder != 0 || v->extracted_units != 0 ||
			!v->vehstatus.Test(VehState::Stopped) || (s.placement != SpacePlacement::InTransit && !v->vehstatus.Test(VehState::Hidden)) ||
			((model == 15 || model == 19 || s.placement == SpacePlacement::Assembling || (s.placement == SpacePlacement::InTransit && !freight_flight)) && v->cargo.TotalCount() != 0)) return false;
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
bool ValidateSpaceProduction()
{
	const auto records = CaptureSpaceProduction();
	return ValidateSpaceProductionRecords(records, Map::SizeX(), Map::SizeY()) && Physical(records, true);
}
bool InstallLoadedSpaceProduction(std::span<const SpaceProductionRecord> records)
{
	if (!ValidateSpaceProductionRecords(records, Map::SizeX(), Map::SizeY()) || !Physical(records, false)) return false;
	for (const auto &record : records) ResolveSpaceCraft(record.craft)->construction = record.state;
	return true;
}
