/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_production.h Native paid hull placement and receipt transport; Plan9 Group8B. */
#ifndef SPACE_PRODUCTION_H
#define SPACE_PRODUCTION_H
#include "launch_center.h"
#include "vehicle_type.h"
#include "engine_type.h"

struct SpaceVehicle;
struct SpaceCraftRef {
	VehicleID vehicle = VehicleID::Invalid();
	uint64_t generation = 0;
	bool operator==(const SpaceCraftRef &) const = default;
};
enum class SpacePlacement : uint8_t { Orbital, Assembling, SurfaceStored, Unhomed, Carried, InTransit, End };
/** Native Vehicle owns work and one consumed-material receipt; center slots hold references only. */
struct SpaceConstructionState {
	SpacePlacement placement = SpacePlacement::Orbital;
	LaunchCenterRef home, origin;
	TileIndex surface_tile = INVALID_TILE;
	uint16_t remaining_ticks = 0, content_version = 0, model = 0;
	uint32_t capital_paid = 0;
	bool paused = false;
	bool deployed = false; ///< Set only when the same carried paid probe is activated in Space.
	bool operator==(const SpaceConstructionState &) const = default;
};
struct SpaceProductionRecord { SpaceCraftRef craft; SpaceConstructionState state; };
std::vector<SpaceProductionRecord> CaptureSpaceProduction();
bool ValidateSpaceProductionRecords(std::span<const SpaceProductionRecord> records, uint32_t width, uint32_t height);
bool ValidateSpaceProduction();
bool InstallLoadedSpaceProduction(std::span<const SpaceProductionRecord> records);
void ResetSpaceProductionLoadStaging();
void ValidateSpaceProductionLoadPreview();
bool AcceptLoadedSpaceProduction();
SpaceCraftRef GetSpaceCraftReference(const SpaceVehicle &vehicle);
SpaceVehicle *ResolveSpaceCraft(SpaceCraftRef ref);
uint16_t GetSpaceVehicleModel(const SpaceVehicle &vehicle);
EngineID GetSpaceEngine(uint16_t model);
bool EnsureSpaceEngineDefinition(uint16_t model);
#endif
