/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_contract.cpp Compile-time public launch/hub/descent handoff checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group7.
 * Downstream mining and construction consume these exact generation-qualified
 * interfaces. A declaration change must update the shared contract explicitly.
 */
#include "../stdafx.h"
#include "../launch_center.h"
#include "../launch_operations.h"
#include "../space_production.h"
#include <type_traits>
#include "../safeguards.h"

/** Both release stages require explicit craft, target and paid-shell identities plus whole-unit quantity. */
using OrbitalCargoRelease = bool (*)(SpaceCraftRef, LaunchCenterRef, SpacePodRef, uint16_t);
static_assert(std::is_same_v<decltype(&CanReleaseOrbitalCargo), OrbitalCargoRelease>);
static_assert(std::is_same_v<decltype(&CommitOrbitalCargoRelease), OrbitalCargoRelease>);

/** Surface handling names the exclusive member link and direction, independently of ordinary catchment. */
using SurfaceCargoHandling = bool (*)(LaunchCenterRef, LaunchHubMemberRef, CargoType, uint16_t, bool);
static_assert(std::is_same_v<decltype(&CanStartSurfaceHandling), SurfaceCargoHandling>);
static_assert(std::is_same_v<decltype(&StartSurfaceHandling), SurfaceCargoHandling>);

/** Runtime jobs contain the real packet owner; copied save records carry references only. */
static_assert(std::is_same_v<decltype(SurfaceHandlingJob::packets), StationCargoList>);
static_assert(std::is_same_v<decltype(SpaceDescentJob::packets), StationCargoList>);
static_assert(std::is_same_v<decltype(SpaceDescentRecord::packets), std::vector<std::pair<CargoPacketID, StationID>>>);
static_assert(std::is_same_v<decltype(SurfaceHandlingRecord::packets), std::vector<std::pair<CargoPacketID, StationID>>>);
static_assert(!std::is_copy_constructible_v<SpaceDescentJob>);
static_assert(!std::is_copy_assignable_v<SpaceDescentJob>);

/** Station owns timed handling, while operational slots reference the one paid native hull. */
static_assert(std::is_same_v<decltype(LaunchCollectionHub::handling), std::vector<std::unique_ptr<SurfaceHandlingJob>>>);
static_assert(std::is_same_v<decltype(LaunchOperations::pad_queue), std::vector<SpaceCraftRef>>);
static_assert(std::is_same_v<decltype(LaunchOperations::pad_active), SpaceCraftRef>);
static_assert(std::is_same_v<decltype(LaunchOperations::workshop), SpaceCraftRef>);

/** Stale-reference resolution returns the existing native owner rather than manufacturing a replacement. */
static_assert(std::is_same_v<decltype(&ResolveLaunchCenter), Station *(*)(LaunchCenterRef)>);
static_assert(std::is_same_v<decltype(&ResolveSpaceCraft), SpaceVehicle *(*)(SpaceCraftRef)>);
static_assert(std::is_same_v<decltype(&GetLaunchHubStorage), Station *(*)(LaunchCenterRef)>);
