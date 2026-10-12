/*
 * This file is part of OpenTTD, licensed under the GNU GPL version 2.
 */
/** @file launch_center.h Native Surface launch-center placement and separate service geometry. */
#ifndef LAUNCH_CENTER_H
#define LAUNCH_CENTER_H
#include "tilearea_orthogonal.h"
#include "space.h"
#include "station_type.h"
#include <functional>
#include <optional>
#include <span>
#include <vector>

struct Station;

/** Native Station owns this metadata. Joining tiles never moves the build anchor. */
struct LaunchCenterAnchor {
	TileIndex tile = INVALID_TILE;
	uint64_t generation = 0; ///< Monotonic world identity; a reused StationID is a different center.
	uint8_t rotation = 0;
};

struct LaunchCenterRef {
	StationID station = StationID::Invalid();
	uint64_t generation = 0;
	bool operator==(const LaunchCenterRef &) const = default;
};

/** Link identity changes on unlink/reselection, even when the native StationID is reused. */
struct LaunchHubMemberRef {
	StationID station = StationID::Invalid();
	uint64_t generation = 0;
	bool operator==(const LaunchHubMemberRef &) const = default;
};

/** Owned by the native center Station. Cargo remains in Station::goods, never in this relationship. */
struct LaunchCollectionHub {
	std::vector<LaunchHubMemberRef> members; ///< Sorted, explicit, exclusive separate-Station memberships.
};

struct LaunchHubMembership {
	LaunchCenterRef center;
	LaunchHubMemberRef member;
	bool joined = false; ///< Same native Station: normal pickup, without a Surface handling transfer.
};

/** Bounded save transport; live ownership remains with Station::launch_hub. */
struct LaunchHubRecord {
	LaunchCenterRef center;
	std::vector<LaunchHubMemberRef> members;
};

/** Bounded save transport, not an independently active station registry. */
struct LaunchCenterRecord {
	StationID station = StationID::Invalid();
	LaunchCenterAnchor anchor;
};

inline constexpr size_t MAX_LAUNCH_CENTERS = 64;
inline constexpr size_t MAX_LAUNCH_HUB_MEMBERS = 32; ///< Separate stations per center; joined facilities use no slot.

inline constexpr uint16_t LAUNCH_CENTER_DROP_RADIUS = 20;
inline constexpr uint16_t LAUNCH_CENTER_COLLECTION_RADIUS = 20;
bool IsLaunchCenterTechnologyAvailable();

/** Rotated 4x3 land extent; validates every coordinate stays on Surface. */
std::optional<TileArea> GetLaunchCenterFootprint(TileIndex anchor, uint8_t rotation);
/** Flat clear navigable sea/canal/river only, with a guarded Surface tile read. */
bool IsLaunchCenterNavigableWater(TileIndex tile);
/** Orthogonal outer edges only. Exclusion supports last-water preflight. */
bool HasLaunchCenterWaterAccess(const TileArea &footprint, TileIndex excluded = INVALID_TILE);
/** Immutable build anchor's layer-local tile center, never the station bounding box. */
std::optional<SpacePoint> GetLaunchCenterPlaneReference(TileIndex anchor);
/** Geometry queries do not authorize a cargo transfer or change any owner. */
bool IsInsideLaunchCenterDropZone(SpacePoint point, SpacePoint anchor);
bool IsInsideLaunchCenterCollectionArea(SpacePoint point, SpacePoint anchor);

Station *ResolveLaunchCenter(LaunchCenterRef ref);
LaunchCenterRef GetLaunchCenterReference(const Station &station);
std::span<const LaunchCenterRef> GetLaunchCenterReferences(); ///< Bounded derived lookup, rebuilt from native Station owners on load.
bool CanAllocateLaunchCenter();
void InstallNewLaunchCenter(Station &station, TileIndex anchor, uint8_t rotation);
void ForgetLaunchCenter(Station &station);
void ResetLaunchCenters();
uint64_t GetNextLaunchCenterGeneration();
std::vector<LaunchCenterRecord> CaptureLaunchCenters();
bool ValidateLaunchCenterRecords(std::span<const LaunchCenterRecord> records, uint64_t next_generation, uint32_t width, uint32_t height);
bool ValidateLaunchCenters();
bool InstallLoadedLaunchCenters(std::vector<LaunchCenterRecord> records, uint64_t next_generation);
uint64_t ComputeLaunchCenterDigest();
/** Tests actual full native footprint, Surface/company and rail/lorry capability; ignores catchment/sign position. */
bool IsLaunchHubEligible(const Station &center, const Station &member);
std::optional<LaunchHubMembership> GetLaunchHubMembership(StationID member);
Station *ResolveLaunchHubMember(LaunchCenterRef center, LaunchHubMemberRef member);
std::vector<LaunchCenterRef> GetEligibleLaunchHubs(const Station &member);
/** One canonical native station inventory, including joined freight facilities. */
Station *GetLaunchHubStorage(LaunchCenterRef center);
uint64_t GetNextLaunchHubMemberGeneration();
std::vector<LaunchHubRecord> CaptureLaunchHubs();
bool ValidateLaunchHubRecords(std::span<const LaunchHubRecord> records, uint64_t next_generation);
bool ValidateLaunchHubs();
bool InstallLoadedLaunchHubs(std::vector<LaunchHubRecord> records, uint64_t next_generation);
uint64_t ComputeLaunchHubDigest();
void ResetLaunchHubs();
/** Native footprint/owner mutation boundary. Invalid links are retired before dispatch; cargo is untouched. */
void RevalidateLaunchHubMemberships();
/** Runs before native Station destruction can release/reuse its pool slot. */
void ForgetLaunchHubStation(Station &station);
/** Pure preflight over the complete proposed removal set, including area commands. */
bool CanRemoveLaunchCenterWater(const std::function<bool(TileIndex)> &removed);

/** Native canal conversion restores qualifying flat water in the same commit. */
class ScopedLaunchCenterWaterReplacement {
	TileIndex previous;
public:
	explicit ScopedLaunchCenterWaterReplacement(TileIndex tile);
	~ScopedLaunchCenterWaterReplacement();
	ScopedLaunchCenterWaterReplacement(const ScopedLaunchCenterWaterReplacement &) = delete;
	ScopedLaunchCenterWaterReplacement &operator=(const ScopedLaunchCenterWaterReplacement &) = delete;
};

void ResetLaunchCenterLoadStaging();
bool AcceptLoadedLaunchCenters();
bool AcceptLoadedLaunchHubs();
void ValidateLaunchCenterLoadPreview();
#endif /* LAUNCH_CENTER_H */
