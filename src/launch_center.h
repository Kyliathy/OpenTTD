/*
 * This file is part of OpenTTD, licensed under the GNU GPL version 2.
 */
/** @file launch_center.h Native Surface launch-center placement and separate service geometry. */
#ifndef LAUNCH_CENTER_H
#define LAUNCH_CENTER_H
#include "tilearea_orthogonal.h"
#include "space.h"
#include "station_type.h"
#include "cargopacket.h"
#include "space_pod_type.h"
#include "vehicle_type.h"
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <vector>

struct Station;
struct SpaceCraftRef;
struct SpacePodRecord;
struct SpaceManifestRecord;
struct LaunchCenterRecord;

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

enum class SpaceDescentBlock : uint8_t { None, TargetLost, StorageFull, CompanyRemoved, End };
/** Owned by the one paid passive pod until its cargo reaches actual Surface inventory. */
struct SpaceDescentJob {
	SpaceDescentJob() = default;
	SpaceDescentJob(const SpaceDescentJob &) = delete;
	SpaceDescentJob &operator=(const SpaceDescentJob &) = delete;
	uint64_t id = 0;
	LaunchCenterRef center;
	SpacePoint release_point{};
	VehicleID sender = VehicleID::Invalid();
	uint64_t sender_generation = 0;
	CargoType cargo = INVALID_CARGO;
	uint16_t quantity = 0, remaining_ticks = 0, age_ticks = 0;
	SpaceDescentBlock blocked = SpaceDescentBlock::None;
	StationCargoList packets{};
};
struct SpaceDescentRecord {
	SpacePodRef pod;
	uint64_t id = 0;
	LaunchCenterRef center;
	SpacePoint release_point{};
	VehicleID sender = VehicleID::Invalid();
	uint64_t sender_generation = 0;
	CargoType cargo = INVALID_CARGO;
	uint16_t quantity = 0, remaining_ticks = 0, age_ticks = 0;
	SpaceDescentBlock blocked = SpaceDescentBlock::None;
	std::vector<std::pair<CargoPacketID, StationID>> packets;
};
bool CanReleaseOrbitalCargo(SpaceCraftRef craft, LaunchCenterRef center, SpacePodRef pod, uint16_t quantity);
bool CommitOrbitalCargoRelease(SpaceCraftRef craft, LaunchCenterRef center, SpacePodRef pod, uint16_t quantity);
bool CanRetargetSpaceDescent(SpacePodRef pod, LaunchCenterRef center);
void TickSpaceDescents();
void ForgetSpaceDescentCenter(LaunchCenterRef center);
uint GetIncomingSpaceDescent(LaunchCenterRef center, CargoType cargo);
bool HasSpaceDescentForCenter(LaunchCenterRef center);
uint64_t GetNextSpaceDescentID();
void ResetSpaceDescents();
std::vector<SpaceDescentRecord> CaptureSpaceDescents();
bool ValidateSpaceDescentRecords(std::span<const SpaceDescentRecord> records, uint64_t next, uint32_t width, uint32_t height);
bool ValidateSpaceDescentLinks(std::span<const SpaceDescentRecord> records, std::span<const SpacePodRecord> pods,
	std::span<const SpaceManifestRecord> manifests, std::span<const LaunchCenterRecord> centers, uint32_t width);
bool ValidateSpaceDescents();
bool InstallLoadedSpaceDescents(std::span<const SpaceDescentRecord> records, uint64_t next);
uint64_t ComputeSpaceDescentDigest();
void ResetSpaceDescentLoadStaging();
bool AcceptLoadedSpaceDescents();
void ValidateSpaceDescentLoadPreview();

/** Link identity changes on unlink/reselection, even when the native StationID is reused. */
struct LaunchHubMemberRef {
	StationID station = StationID::Invalid();
	uint64_t generation = 0;
	bool operator==(const LaunchHubMemberRef &) const = default;
};

/** Saved timed job owns these actual packets until delivery or cargo-preserving settlement. */
struct SurfaceHandlingJob {
	uint64_t id = 0;
	LaunchCenterRef center;
	LaunchHubMemberRef member;
	CargoType cargo = INVALID_CARGO;
	bool reverse = false; ///< Only an explicit supply transfer may travel towards the center.
	uint16_t remaining_ticks = 0;
	uint16_t age_ticks = 0;
	StationCargoList packets{};
};

/** Native Station owns relationships and jobs; ordinary waiting stock remains in Station::goods. */
struct LaunchCollectionHub {
	std::vector<LaunchHubMemberRef> members; ///< Sorted, explicit, exclusive separate-Station memberships.
	std::vector<std::unique_ptr<SurfaceHandlingJob>> handling; ///< Ascending monotonic job ID.
	uint8_t handling_cost_remainder = 0; ///< Sixteenths of a tonne; Goods crates must not round down per packet.
};

struct SurfaceHandlingRecord {
	uint64_t id = 0;
	LaunchCenterRef center;
	LaunchHubMemberRef member;
	CargoType cargo = INVALID_CARGO;
	bool reverse = false;
	uint16_t remaining_ticks = 0, age_ticks = 0;
	std::vector<std::pair<CargoPacketID, StationID>> packets;
};

struct SurfaceHandlingCostRecord {
	LaunchCenterRef center;
	uint8_t remainder = 0;
};

inline constexpr uint16_t LAUNCH_HANDLING_THROUGHPUT = 8;
inline constexpr uint16_t LAUNCH_HANDLING_STOCK_CAP = 1024;
inline constexpr uint16_t MAX_SURFACE_HANDLING_QUANTITY = 64;
inline constexpr size_t MAX_SURFACE_HANDLING_JOBS = 32; ///< Per center, independent of member count.

bool CanStartSurfaceHandling(LaunchCenterRef center, LaunchHubMemberRef member, CargoType cargo, uint16_t quantity, bool reverse);
bool IsLaunchSupplyCargo(CargoType cargo);
uint GetLaunchStockCapacity(const Station &station, CargoType cargo);
/** Existing waiting packets and incoming Surface jobs reserve the same physical capacity. */
uint GetLaunchStockHeadroom(const Station &station, CargoType cargo);
bool StartSurfaceHandling(LaunchCenterRef center, LaunchHubMemberRef member, CargoType cargo, uint16_t quantity, bool reverse);
void TickSurfaceHandling();
bool HasSurfaceHandlingForStation(StationID station);
void SettleSurfaceHandlingForStation(Station &station, bool cleaning_pool = false);
bool ValidateSurfaceHandling();
uint64_t GetNextSurfaceHandlingID();
std::vector<SurfaceHandlingRecord> CaptureSurfaceHandling();
std::vector<SurfaceHandlingCostRecord> CaptureSurfaceHandlingCosts();
bool ValidateSurfaceHandlingRecords(std::span<const SurfaceHandlingRecord> records, std::span<const SurfaceHandlingCostRecord> costs, uint64_t next);
bool InstallLoadedSurfaceHandling(std::span<const SurfaceHandlingRecord> records, std::span<const SurfaceHandlingCostRecord> costs, uint64_t next);
void ResetSurfaceHandling();
uint64_t ComputeSurfaceHandlingDigest();

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
bool AcceptLoadedSurfaceHandling();
void ValidateLaunchCenterLoadPreview();
#endif /* LAUNCH_CENTER_H */
