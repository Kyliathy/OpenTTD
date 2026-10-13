/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_pod.h Native passive pod pool, paid construction and single custody. */
#ifndef SPACE_POD_H
#define SPACE_POD_H
#include "space_pod_type.h"
#include "space_production.h"
#include "company_type.h"

struct SpacePod;
struct LaunchOperationsRecord;
struct SpaceManifestRecord;
using SpacePodPool = Pool<SpacePod, SpacePodID, 16>;
extern SpacePodPool _space_pod_pool;

enum class SpacePodCustody : uint8_t { Assembling, CenterStored, Craft, Unhomed, Drifting, Descent, End };
/** Actual paid object. Center/craft slots hold typed references, never shell copies. */
struct SpacePodState {
	Owner owner = OWNER_NONE;
	SpacePodCustody custody = SpacePodCustody::Unhomed;
	LaunchCenterRef home, origin;
	SpaceCraftRef craft;
	TileIndex surface_tile = INVALID_TILE;
	SpacePoint position{};
	uint16_t remaining_ticks = 0, content_version = 0;
	uint32_t capital_paid = 0;
	bool paused = false;
	bool operator==(const SpacePodState &) const = default;
};
struct SpacePod : SpacePodPool::PoolItem<&_space_pod_pool> {
	uint64_t generation = 0;
	SpacePodState state;
	std::unique_ptr<SpaceDescentJob> descent;
	SpacePod(SpacePodID index) : SpacePodPool::PoolItem<&_space_pod_pool>(index) {}
	~SpacePod();
};
struct SpacePodRecord { SpacePodRef ref; SpacePodState state; };
SpacePodRef GetSpacePodReference(const SpacePod &pod);
SpacePod *ResolveSpacePod(SpacePodRef ref);
uint64_t GetNextSpacePodGeneration();
uint CountLaunchPods(LaunchCenterRef center);
bool HasLaunchPods(LaunchCenterRef center);
void TickSpacePods();
void SyncSpacePods(const SpaceVehicle &craft);
void ForgetSpacePodCraft(const SpaceVehicle &craft);
void ForgetSpacePodCenter(LaunchCenterRef center);
void ChangeSpacePodOwnership(Owner old_owner, Owner new_owner);
void ResetSpacePods();
std::vector<SpacePodRecord> CaptureSpacePods();
bool ValidateSpacePodRecords(std::span<const SpacePodRecord> records, uint64_t next_generation, uint32_t width, uint32_t height);
bool ValidateSpacePodLinks(std::span<const SpacePodRecord> pods, std::span<const SpaceProductionRecord> craft,
	std::span<const LaunchOperationsRecord> operations, std::span<const SpaceManifestRecord> manifests);
bool ValidateSpacePods();
bool InstallLoadedSpacePods(std::span<const SpacePodRecord> records, uint64_t next_generation);
uint64_t ComputeSpacePodDigest();
void ResetSpacePodLoadStaging();
bool AcceptLoadedSpacePods();
void ValidateSpacePodLoadPreview();
#endif
