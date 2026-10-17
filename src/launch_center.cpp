/*
 * This file is part of OpenTTD, licensed under the GNU GPL version 2.
 */
/** @file launch_center.cpp Guarded native placement queries; no camera or wrapped neighbours.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Groups1-3
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group2
 */
#include "stdafx.h"
#include "space_order_actions.h"
#include "launch_center.h"
#include "launch_site.h"
#include "launch_operations.h"
#include "map_layer.h"
#include "water_map.h"
#include "bridge_map.h"
#include "station_base.h"
#include "station_map.h"
#include "company_base.h"
#include "multilayer_content.h"
#include "timer/timer_game_calendar.h"
#include <set>
#include "safeguards.h"

static uint64_t _next_launch_center_generation = 1;
/** Derived bounded lookup only. The native Station owns every anchor. */
static std::vector<LaunchCenterRef> _launch_centers;
static TileIndex _launch_center_replaced_water = INVALID_TILE;

/** @param tile Explicit water tile replaced by a native operation; the previous scope is retained for restoration. */
ScopedLaunchCenterWaterReplacement::ScopedLaunchCenterWaterReplacement(TileIndex tile) : previous(_launch_center_replaced_water) { _launch_center_replaced_water = tile; }
/** Restore the enclosing replacement scope after native placement/water validation completes. */
ScopedLaunchCenterWaterReplacement::~ScopedLaunchCenterWaterReplacement() { _launch_center_replaced_water = this->previous; }

/** @return Whether selected profile and calendar technology admit paid launch construction; geometry alone grants no permission. */
bool IsLaunchCenterTechnologyAvailable()
{
	return GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry && TimerGameCalendar::year >= TimerGameCalendar::Year{1990};
}

/** @param area Proposed complete footprint. @return Whether a nonempty explicit Surface extent remains inside the actual plane. */
static bool IsSurfaceExtent(const TileArea &area)
{
	// Empty or cross-layer extents cannot own launch zones, and bounded coordinates must never wrap to another plane.
	if (area.w == 0 || area.h == 0 || !MultiLayerMap::IsTileInLayer(area.tile, MapLayer::Surface)) return false;
	return static_cast<uint64_t>(TileX(area.tile)) + area.w <= Map::SizeX() &&
		static_cast<uint64_t>(TilePlaneY(area.tile)) + area.h <= Map::SizeY();
}

/**
 * Return the complete rotated Surface footprint using the shared immutable native dimensions.
 * @param anchor Surface build anchor; cross-layer and out-of-plane extents refuse.
 * @param rotation Quarter-turn native layout in [0, 4).
 * @return Guarded physical footprint, or none when rotation/layer/bounds are invalid.
 */
std::optional<TileArea> GetLaunchCenterFootprint(TileIndex anchor, uint8_t rotation)
{
	if (rotation >= 4) return std::nullopt;
	const TileArea area(anchor, rotation % 2 == 0 ? LAUNCH_CENTER_LONG_EDGE : LAUNCH_CENTER_SHORT_EDGE,
		rotation % 2 == 0 ? LAUNCH_CENTER_SHORT_EDGE : LAUNCH_CENTER_LONG_EDGE);
	return IsSurfaceExtent(area) ? std::optional{area} : std::nullopt;
}

/** @param tile Explicit world tile to inspect. @return Whether it is flat navigable Surface sea, canal or river. */
bool IsLaunchCenterNavigableWater(TileIndex tile)
{
	// Only actual flat Surface water can service the footprint; matching projected coordinates or a sloped subtype cannot qualify.
	if (!MultiLayerMap::IsTileInLayer(tile, MapLayer::Surface) || !IsWaterTile(tile) || !IsTileFlat(tile)) return false;
	return IsSea(tile) || IsCanal(tile) || IsRiver(tile);
}

/** @param footprint Guarded Surface extent. @param excluded Removed/replaced tile, or INVALID_TILE. @return Whether an orthogonal outer edge retains navigable water. */
bool HasLaunchCenterWaterAccess(const TileArea &footprint, TileIndex excluded)
{
	if (!IsSurfaceExtent(footprint)) return false;
	const int64_t x = TileX(footprint.tile), y = TilePlaneY(footprint.tile);
	/** @param nx Proposed edge x coordinate. @param ny Proposed edge y coordinate. @return Surviving guarded navigable water. */
	auto water = [&](int64_t nx, int64_t ny) {
		// Edge tests cannot wrap at a map boundary and accidentally authorize water in another layer or opposite corner.
		if (nx < 0 || ny < 0 || nx >= Map::SizeX() || ny >= Map::SizeY()) return false;
		const auto tile = TileXYL(static_cast<uint>(nx), static_cast<uint>(ny), MapLayer::Surface);
		return tile != excluded && IsLaunchCenterNavigableWater(tile);
	};
	// Inspect only orthogonal perimeter cells; diagonal/internal water cannot supply physical waterside access.
	for (uint dx = 0; dx < footprint.w; ++dx) if (water(x + dx, y - 1) || water(x + dx, y + footprint.h)) return true;
	for (uint dy = 0; dy < footprint.h; ++dy) if (water(x - 1, y + dy) || water(x + footprint.w, y + dy)) return true;
	return false;
}

/** @param anchor Immutable Surface build anchor. @return Its projected Space reference, or none for another layer/invalid tile. */
std::optional<SpacePoint> GetLaunchCenterPlaneReference(TileIndex anchor)
{
	if (!MultiLayerMap::IsTileInLayer(anchor, MapLayer::Surface)) return std::nullopt;
	return SpacePoint{static_cast<int32_t>(TileX(anchor)) * SPACE_Q16, static_cast<int32_t>(TilePlaneY(anchor)) * SPACE_Q16};
}

/** @param point Projected entity point. @param anchor Immutable projected facility reference. @param radius Inclusive native radius in tiles. @return Circular membership without overflow or wrapping. */
static bool InsideCircle(SpacePoint point, SpacePoint anchor, uint16_t radius)
{
	const int64_t dx = int64_t{point.x} - anchor.x, dy = int64_t{point.y} - anchor.y;
	const int64_t r = int64_t{radius} * SPACE_Q16;
	// Reject impossible axis extents before squaring so hostile coordinates cannot overflow into an eligible circle.
	if (dx < -r || dx > r || dy < -r || dy > r) return false;
	return dx * dx + dy * dy <= r * r;
}

/** @param point Current orbital point. @param anchor Immutable facility reference. @return Inclusive orbital release-circle membership only. */
bool IsInsideLaunchCenterDropZone(SpacePoint point, SpacePoint anchor)
{
	return InsideCircle(point, anchor, LAUNCH_CENTER_DROP_RADIUS);
}

/** @param point Projected Surface member point. @param anchor Immutable facility reference. @return Independent Surface collection-circle membership only. */
bool IsInsideLaunchCenterCollectionArea(SpacePoint point, SpacePoint anchor)
{
	return InsideCircle(point, anchor, LAUNCH_CENTER_COLLECTION_RADIUS);
}

/** @param ref Explicit station and generation. @return That surviving native facility, or nullptr for missing/reused identities. */
Station *ResolveLaunchCenter(LaunchCenterRef ref)
{
	auto *station = Station::GetIfValid(ref.station);
	return station != nullptr && station->launch_center != nullptr && station->launch_center->generation == ref.generation ? station : nullptr;
}

/** @param station Actual native station owner. @return Its current facility incarnation, or an invalid reference when no facility exists. */
LaunchCenterRef GetLaunchCenterReference(const Station &station)
{
	return station.launch_center == nullptr ? LaunchCenterRef{} : LaunchCenterRef{station.index, station.launch_center->generation};
}

/** @return Whether bounded native facility capacity and the monotonic generation counter can admit one more center. */
bool CanAllocateLaunchCenter()
{
	return _launch_centers.size() < MAX_LAUNCH_CENTERS && _next_launch_center_generation != UINT64_MAX;
}

/** @return Read-only bounded facility lookup; actual Station objects remain the sole anchor owners. */
std::span<const LaunchCenterRef> GetLaunchCenterReferences() { return _launch_centers; }

/** @param station Actual newly built station. @param anchor Immutable Surface reference. @param rotation Native physical-zone layout. */
void InstallNewLaunchCenter(Station &station, TileIndex anchor, uint8_t rotation)
{
	assert(station.launch_center == nullptr && CanAllocateLaunchCenter());
	station.launch_center = std::make_unique<LaunchCenterAnchor>(anchor, _next_launch_center_generation++, rotation);
	station.launch_hub = std::make_unique<LaunchCollectionHub>();
	station.launch_operations = std::make_unique<LaunchOperations>();
	_launch_centers.push_back(GetLaunchCenterReference(station));
	/** @param ref One native facility incarnation. @return Stable station key for deterministic arbitration. */
	std::ranges::sort(_launch_centers, {}, [](const auto &ref) { return ref.station.base(); });
}

/** @param station Actual station losing its facility; shared operation/hub owners settle their custody before anchor retirement. */
void ForgetLaunchCenter(Station &station)
{
	ForgetLaunchOperations(station, Station::CleaningPool());
	ForgetLaunchHubStation(station);
	if (station.launch_center == nullptr) return;
	const auto ref = GetLaunchCenterReference(station);
	if (!Station::CleaningPool()) ForgetSpaceServiceOrderCenter(ref); // Actual retirement marks only existing onboard work; bulk teardown must not inspect already cleaned vehicle owners.
	std::erase(_launch_centers, ref);
	station.launch_center.reset();
}

/** Clear facility owners and disposable lookup generations before another world is initialized. */
void ResetLaunchCenters()
{
	ResetLaunchHubs();
	// Initialization resets every actual station owner rather than leaving dangling anchors in a second registry.
	for (auto *station : Station::Iterate()) { station->launch_center.reset(); station->launch_operations.reset(); }
	_launch_centers.clear();
	_next_launch_center_generation = 1;
	_launch_center_replaced_water = INVALID_TILE;
	ResetLaunchCenterLoadStaging();
}

/** @return Saved monotonic next-generation identity; removed facility incarnations are never reused. */
uint64_t GetNextLaunchCenterGeneration() { return _next_launch_center_generation; }

/** @return Copied transport rows from the actual native station owners, without transferring gameplay authority. */
std::vector<LaunchCenterRecord> CaptureLaunchCenters()
{
	std::vector<LaunchCenterRecord> result;
	// Capture only real anchors; empty ordinary stations never become launch facilities through serialization.
	for (const auto *station : Station::Iterate())
		if (station->launch_center != nullptr) result.push_back({station->index, *station->launch_center});
	return result;
}

/** @param records Copied saved anchors. @param next Saved next generation. @param width Saved plane width. @param height Saved plane height. @return Structural identity/extent validity without current-world mutation. */
bool ValidateLaunchCenterRecords(std::span<const LaunchCenterRecord> records, uint64_t next, uint32_t width, uint32_t height)
{
	// Oversized identity sets, exhausted/invalid counters or unsupported dimensions cannot own bounded physical facilities.
	if (records.size() > MAX_LAUNCH_CENTERS || next == 0 || width < 64 || height < 64 || width > 4096 || height > 4096) return false;
	std::set<StationID> stations;
	std::set<uint64_t> generations;
	std::set<TileIndex> occupied;
	// Validate unique station/generation identities and every claimed footprint before any load publication.
	for (const auto &record : records) {
		const auto &anchor = record.anchor;
		// Missing/duplicate identities, bad layouts or out-of-plane anchors would alias ownership or overlap native facilities.
		if (record.station >= StationID::End() || anchor.generation == 0 || anchor.generation >= next || anchor.rotation >= 4 ||
			anchor.tile.base() >= uint64_t{width} * height || !stations.insert(record.station).second || !generations.insert(anchor.generation).second) return false;
		const uint32_t w = anchor.rotation % 2 == 0 ? LAUNCH_CENTER_LONG_EDGE : LAUNCH_CENTER_SHORT_EDGE;
		const uint32_t h = anchor.rotation % 2 == 0 ? LAUNCH_CENTER_SHORT_EDGE : LAUNCH_CENTER_LONG_EDGE;
		// The entire native extent must remain in this saved plane, not just its anchor tile.
		if (anchor.tile.base() % width + w > width || anchor.tile.base() / width + h > height) return false;
		// Reject any repeated physical tile claim even when two anchor records otherwise appear individually legal.
		for (uint32_t dy = 0; dy < h; ++dy) for (uint32_t dx = 0; dx < w; ++dx)
			if (!occupied.insert(TileIndex{anchor.tile.base() + dy * width + dx}).second) return false;
	}
	return true;
}

/** @param records Structurally valid saved anchors. @return Whether their real stations/tiles and physical water edges agree. */
static bool ValidatePhysicalLaunchCenters(std::span<const LaunchCenterRecord> records)
{
	// Classic has no native launch producer, so a nonempty facility set cannot be accepted into that profile.
	if (!records.empty() && GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return false;
	std::set<StationID> found;
	// Resolve each actual owner and complete native footprint before accepting its operational geometry.
	for (const auto &record : records) {
		const auto *station = Station::GetIfValid(record.station);
		const auto area = GetLaunchCenterFootprint(record.anchor.tile, record.anchor.rotation);
		// A missing company/station/facility or lost water/extent cannot be replaced by a copied anchor record.
		if (station == nullptr || !Company::IsValidID(station->owner) || !station->facilities.Test(StationFacility::LaunchCenter) ||
			!area || !HasLaunchCenterWaterAccess(*area)) return false;
		// Every zone tile must belong to the same real owner/layout and remain physically serviceable.
		for (const auto tile : *area) {
			// Wrong station/type/owner, slope/layout mismatch or an overhead bridge invalidates the native facility.
			if (!IsTileType(tile, TileType::Station) || GetStationType(tile) != StationType::LaunchCenter ||
				GetStationIndex(tile) != station->index || GetTileOwner(tile) != station->owner || !IsTileFlat(tile) ||
				GetStationGfx(tile) != record.anchor.rotation || IsBridgeAbove(tile)) return false;
		}
		found.insert(station->index);
	}
	// Native facility bits must correspond exactly to retained anchors rather than introducing an orphan station owner.
	for (const auto *station : Station::Iterate())
		if (station->facilities.Test(StationFacility::LaunchCenter) != found.contains(station->index)) return false;
	// Once per load/save, rather than per tick: reject orphan or extra facility tiles.
	for (const auto tile : Map::Iterate()) {
		// Ordinary station tiles and other types have no launch-zone claim to validate in this pass.
		if (!IsTileType(tile, TileType::Station) || GetStationType(tile) != StationType::LaunchCenter) continue;
		const auto it = std::ranges::find(records, GetStationIndex(tile), &LaunchCenterRecord::station);
		if (it == records.end()) return false;
		const auto area = GetLaunchCenterFootprint(it->anchor.tile, it->anchor.rotation);
		// A real anchor cannot authorize extra facility tiles outside its complete physical extent.
		if (!area || !area->Contains(tile)) return false;
	}
	return true;
}

/** @return Whether current copied/physical owners and every derived generation-qualified lookup agree. */
bool ValidateLaunchCenters()
{
	const auto records = CaptureLaunchCenters();
	// Structural, physical and derived lookup counts must agree before save/load consumers trust facility ownership.
	if (!ValidateLaunchCenterRecords(records, _next_launch_center_generation, Map::SizeX(), Map::SizeY()) ||
		!ValidatePhysicalLaunchCenters(records) || records.size() != _launch_centers.size()) return false;
	for (const auto &ref : _launch_centers) if (ResolveLaunchCenter(ref) == nullptr) return false;
	return true;
}

/** @param records Complete validated transport rows. @param next Saved monotonic next generation. @return Whether native owners and lookup could be published together. */
bool InstallLoadedLaunchCenters(std::vector<LaunchCenterRecord> records, uint64_t next)
{
	// Invalid structural or physical state must leave the live facility lookup unpublished rather than partially installing it.
	if (!ValidateLaunchCenterRecords(records, next, Map::SizeX(), Map::SizeY()) || !ValidatePhysicalLaunchCenters(records)) return false;
	_launch_centers.clear();
	// Reattach saved anchors only to their actual native station owners; no copied row creates another station pool identity.
	for (const auto &record : records) {
		auto *station = Station::Get(record.station);
		station->launch_center = std::make_unique<LaunchCenterAnchor>(record.anchor);
		station->launch_hub = std::make_unique<LaunchCollectionHub>();
		station->launch_operations = std::make_unique<LaunchOperations>();
		_launch_centers.push_back(GetLaunchCenterReference(*station));
	}
	/** @param ref Retained native facility incarnation. @return Stable station key for deterministic lookup/arbitration. */
	std::ranges::sort(_launch_centers, {}, [](const auto &ref) { return ref.station.base(); });
	_next_launch_center_generation = next;
	return true;
}

/** @return Stable native ownership/production diagnostic digest, excluding disposable renderer data. */
uint64_t ComputeLaunchCenterDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	/** @param value One native saved scalar, appended in stable byte order to expose ownership divergence. */
	auto hash = [&](uint64_t value) { for (uint shift = 0; shift < 64; shift += 8) { digest ^= (value >> shift) & 0xFF; digest *= 1099511628211ULL; } };
	hash(_next_launch_center_generation);
	hash(ComputeLaunchProductionDigest());
	// Include each real station owner and immutable anchor incarnation, not only a matching visible bounding box.
	for (const auto &record : CaptureLaunchCenters()) {
		hash(record.station.base()); hash(Station::Get(record.station)->owner.base());
		hash(record.anchor.generation); hash(record.anchor.tile.base()); hash(record.anchor.rotation);
	}
	return digest;
}

/** @param removed Predicate selecting actual tiles removed by the pending native operation. @return Whether every surviving facility retains a qualifying edge. */
bool CanRemoveLaunchCenterWater(const std::function<bool(TileIndex)> &removed)
{
	// A shared water operation is legal only if every affected actual facility remains serviceable afterward.
	for (const auto ref : _launch_centers) {
		const auto *station = ResolveLaunchCenter(ref);
		if (station == nullptr) return false;
		const auto area = GetLaunchCenterFootprint(station->launch_center->tile, station->launch_center->rotation);
		if (!area) return false;
		bool water_left = false;
		const int x = TileX(area->tile), y = TilePlaneY(area->tile);
		/** @param nx Proposed perimeter x. @param ny Proposed perimeter y. Retain only guarded surviving/replaced navigable water. */
		auto inspect = [&](int nx, int ny) {
			// A perimeter inspection must not wrap to the opposite map edge or another layer.
			if (nx < 0 || ny < 0 || nx >= static_cast<int>(Map::SizeX()) || ny >= static_cast<int>(Map::SizeY())) return;
			const auto tile = TileXYL(nx, ny, MapLayer::Surface);
			// The explicit replacement or an unremoved tile qualifies only when it remains genuine navigable Surface water.
			if ((tile == _launch_center_replaced_water || !removed(tile)) && IsLaunchCenterNavigableWater(tile)) water_left = true;
		};
		// Check the complete orthogonal perimeter; diagonal/internal remnants cannot substitute for physical waterside service.
		for (uint dx = 0; dx < area->w; ++dx) { inspect(x + dx, y - 1); inspect(x + dx, y + area->h); }
		for (uint dy = 0; dy < area->h; ++dy) { inspect(x - 1, y + dy); inspect(x + area->w, y + dy); }
		if (!water_left) return false;
	}
	return true;
}
