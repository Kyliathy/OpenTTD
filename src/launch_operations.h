/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_operations.h Native Station slot authority, geometry and paid assembly; Plan9 Groups2B/8B. */
#ifndef LAUNCH_OPERATIONS_H
#define LAUNCH_OPERATIONS_H
#include "space_production.h"
#include "space_pod_type.h"

enum class LaunchFacilityZone : uint8_t { Pad, Workshop, Recovery, Storage, End };
inline constexpr size_t MAX_CENTER_CRAFT = 8;
/** Station owns reservations, Vehicle owns the actual paid hull and work. */
struct LaunchOperations {
	SpaceCraftRef workshop, service;
	SpacePodRef pod_workshop;
	SpaceCraftRef pad_active;
	std::vector<SpaceCraftRef> pad_queue;
};
struct LaunchOperationsRecord { LaunchCenterRef center; LaunchOperations slots; };
std::optional<TileArea> GetLaunchFacilityZone(TileIndex anchor, uint8_t rotation, LaunchFacilityZone zone);
std::optional<SpacePoint> GetLaunchServiceApproach(const Station &center);
bool IsAtLaunchServiceApproach(SpacePoint point, const Station &center);
bool HasLaunchCraft(LaunchCenterRef center);
void TickLaunchOperations();
void RefreshLaunchPad(Station &center);
void ForgetLaunchOperations(Station &center, bool cleaning_pool);
void ForgetLaunchCraft(SpaceVehicle &vehicle);
std::vector<LaunchOperationsRecord> CaptureLaunchOperations();
bool ValidateLaunchOperationRecords(std::span<const LaunchOperationsRecord> records);
/** Cross-family validation over copied records only; safe for independent preview. */
bool ValidateLaunchProductionLinks(std::span<const SpaceProductionRecord> craft, std::span<const LaunchOperationsRecord> operations,
	std::span<const SpaceCraftRef> identities, std::span<const LaunchCenterRef> centers);
bool ValidateLaunchOperations();
bool InstallLoadedLaunchOperations(std::span<const LaunchOperationsRecord> records);
uint64_t ComputeLaunchProductionDigest();
void ShowLaunchAssemblyWindow(LaunchCenterRef center);
#endif
