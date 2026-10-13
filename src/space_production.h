/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file space_production.h Native paid hull placement and construction receipt transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups2B/8B.
 */
#ifndef SPACE_PRODUCTION_H
#define SPACE_PRODUCTION_H
#include "launch_center.h"
#include "vehicle_type.h"
#include "engine_type.h"

struct SpaceVehicle;
/** Vehicle pool identity plus monotonic generation; a reused pool ID never resolves an old hull. */
struct SpaceCraftRef {
	VehicleID vehicle = VehicleID::Invalid();
	uint64_t generation = 0;
	/** @param other Reference to compare, including its generation. @return Whether both identify the same hull. */
	bool operator==(const SpaceCraftRef &other) const = default;
};
/** Saved physical placement; dormant, carried and transitioning hulls cannot tick as orbital units. */
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
	/** @param other Paid receipt and work state to compare. @return Whether every saved construction field agrees. */
	bool operator==(const SpaceConstructionState &other) const = default;
};
/** Copied SVST transport; the referenced Vehicle remains the sole paid hull and work owner. */
struct SpaceProductionRecord { SpaceCraftRef craft; SpaceConstructionState state; };
/** @return Copied production receipts in native Vehicle order, without changing paid work or placement. */
std::vector<SpaceProductionRecord> CaptureSpaceProduction();
/**
 * Validate copied receipts, paid materials/capital, placement and bounded work without pool access.
 * @param records Staged SVST construction records.
 * @param width Saved plane width in tiles.
 * @param height Saved plane height in tiles.
 * @return Whether all identities and construction states are structurally valid.
 */
bool ValidateSpaceProductionRecords(std::span<const SpaceProductionRecord> records, uint32_t width, uint32_t height);
/** @return Whether current receipts agree with actual native hull models, owners and placement. */
bool ValidateSpaceProduction();
/**
 * Restore paid work on existing native hulls after complete validation.
 * @param records Staged SVST records; refusal leaves current construction states untouched.
 * @return Whether every receipt was validated and installed.
 */
bool InstallLoadedSpaceProduction(std::span<const SpaceProductionRecord> records);
/** Clear staged SVST/LCOP data between loads without granting a hull or rewriting live work. */
void ResetSpaceProductionLoadStaging();
/** Validate copied construction/slot preview data without installing or mutating native owners. */
void ValidateSpaceProductionLoadPreview();
/** @return Whether staged construction and slot families were accepted by their native owners. */
bool AcceptLoadedSpaceProduction();
/** @param vehicle Native hull to identify. @return Its pool ID and monotonic fleet generation. */
SpaceCraftRef GetSpaceCraftReference(const SpaceVehicle &vehicle);
/** @param ref Generation-qualified hull reference. @return Existing native hull, or nullptr for a stale/empty reference. */
SpaceVehicle *ResolveSpaceCraft(SpaceCraftRef ref);
/** @param vehicle Native hull whose Engine supplies its immutable logical model. @return Logical content model identity. */
uint16_t GetSpaceVehicleModel(const SpaceVehicle &vehicle);
/** @param model Immutable logical content model. @return Registered native EngineID, or an invalid identity if absent. */
EngineID GetSpaceEngine(uint16_t model);
/**
 * Register the canonical native Engine definition needed by paid commissioning.
 * @param model Immutable logical content model to resolve or register.
 * @return Whether the real Engine definition exists; this creates no Vehicle or cargo.
 */
bool EnsureSpaceEngineDefinition(uint16_t model);
#endif
