/*
 * This file is part of OpenTTD, licensed under the GNU GPL version 2.
 */
/** @file launch_center.cpp Guarded native placement queries; no camera or wrapped neighbours. */
#include "stdafx.h"
#include "launch_center.h"
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

ScopedLaunchCenterWaterReplacement::ScopedLaunchCenterWaterReplacement(TileIndex tile) : previous(_launch_center_replaced_water) { _launch_center_replaced_water = tile; }
ScopedLaunchCenterWaterReplacement::~ScopedLaunchCenterWaterReplacement() { _launch_center_replaced_water = this->previous; }

bool IsLaunchCenterTechnologyAvailable()
{
	return GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry && TimerGameCalendar::year >= TimerGameCalendar::Year{1990};
}

static bool IsSurfaceExtent(const TileArea &area)
{
	if (area.w == 0 || area.h == 0 || !MultiLayerMap::IsTileInLayer(area.tile, MapLayer::Surface)) return false;
	return static_cast<uint64_t>(TileX(area.tile)) + area.w <= Map::SizeX() &&
		static_cast<uint64_t>(TilePlaneY(area.tile)) + area.h <= Map::SizeY();
}

std::optional<TileArea> GetLaunchCenterFootprint(TileIndex anchor, uint8_t rotation)
{
	if (rotation >= 4) return std::nullopt;
	const TileArea area(anchor, rotation % 2 == 0 ? 4 : 3, rotation % 2 == 0 ? 3 : 4);
	return IsSurfaceExtent(area) ? std::optional{area} : std::nullopt;
}

bool IsLaunchCenterNavigableWater(TileIndex tile)
{
	if (!MultiLayerMap::IsTileInLayer(tile, MapLayer::Surface) || !IsWaterTile(tile) || !IsTileFlat(tile)) return false;
	return IsSea(tile) || IsCanal(tile) || IsRiver(tile);
}

bool HasLaunchCenterWaterAccess(const TileArea &footprint, TileIndex excluded)
{
	if (!IsSurfaceExtent(footprint)) return false;
	const int64_t x = TileX(footprint.tile), y = TilePlaneY(footprint.tile);
	auto water = [&](int64_t nx, int64_t ny) {
		if (nx < 0 || ny < 0 || nx >= Map::SizeX() || ny >= Map::SizeY()) return false;
		const auto tile = TileXYL(static_cast<uint>(nx), static_cast<uint>(ny), MapLayer::Surface);
		return tile != excluded && IsLaunchCenterNavigableWater(tile);
	};
	for (uint dx = 0; dx < footprint.w; ++dx) if (water(x + dx, y - 1) || water(x + dx, y + footprint.h)) return true;
	for (uint dy = 0; dy < footprint.h; ++dy) if (water(x - 1, y + dy) || water(x + footprint.w, y + dy)) return true;
	return false;
}

std::optional<SpacePoint> GetLaunchCenterPlaneReference(TileIndex anchor)
{
	if (!MultiLayerMap::IsTileInLayer(anchor, MapLayer::Surface)) return std::nullopt;
	return SpacePoint{static_cast<int32_t>(TileX(anchor)) * SPACE_Q16, static_cast<int32_t>(TilePlaneY(anchor)) * SPACE_Q16};
}

static bool InsideCircle(SpacePoint point, SpacePoint anchor, uint16_t radius)
{
	const int64_t dx = int64_t{point.x} - anchor.x, dy = int64_t{point.y} - anchor.y;
	const int64_t r = int64_t{radius} * SPACE_Q16;
	if (dx < -r || dx > r || dy < -r || dy > r) return false;
	return dx * dx + dy * dy <= r * r;
}

bool IsInsideLaunchCenterDropZone(SpacePoint point, SpacePoint anchor)
{
	return InsideCircle(point, anchor, LAUNCH_CENTER_DROP_RADIUS);
}

bool IsInsideLaunchCenterCollectionArea(SpacePoint point, SpacePoint anchor)
{
	return InsideCircle(point, anchor, LAUNCH_CENTER_COLLECTION_RADIUS);
}

Station *ResolveLaunchCenter(LaunchCenterRef ref)
{
	auto *station = Station::GetIfValid(ref.station);
	return station != nullptr && station->launch_center != nullptr && station->launch_center->generation == ref.generation ? station : nullptr;
}

LaunchCenterRef GetLaunchCenterReference(const Station &station)
{
	return station.launch_center == nullptr ? LaunchCenterRef{} : LaunchCenterRef{station.index, station.launch_center->generation};
}

bool CanAllocateLaunchCenter()
{
	return _launch_centers.size() < MAX_LAUNCH_CENTERS && _next_launch_center_generation != UINT64_MAX;
}

std::span<const LaunchCenterRef> GetLaunchCenterReferences() { return _launch_centers; }

void InstallNewLaunchCenter(Station &station, TileIndex anchor, uint8_t rotation)
{
	assert(station.launch_center == nullptr && CanAllocateLaunchCenter());
	station.launch_center = std::make_unique<LaunchCenterAnchor>(anchor, _next_launch_center_generation++, rotation);
	station.launch_hub = std::make_unique<LaunchCollectionHub>();
	_launch_centers.push_back(GetLaunchCenterReference(station));
	std::ranges::sort(_launch_centers, {}, [](const auto &ref) { return ref.station.base(); });
}

void ForgetLaunchCenter(Station &station)
{
	ForgetLaunchHubStation(station);
	if (station.launch_center == nullptr) return;
	const auto ref = GetLaunchCenterReference(station);
	std::erase(_launch_centers, ref);
	station.launch_center.reset();
}

void ResetLaunchCenters()
{
	ResetLaunchHubs();
	for (auto *station : Station::Iterate()) station->launch_center.reset();
	_launch_centers.clear();
	_next_launch_center_generation = 1;
	_launch_center_replaced_water = INVALID_TILE;
	ResetLaunchCenterLoadStaging();
}

uint64_t GetNextLaunchCenterGeneration() { return _next_launch_center_generation; }

std::vector<LaunchCenterRecord> CaptureLaunchCenters()
{
	std::vector<LaunchCenterRecord> result;
	for (const auto *station : Station::Iterate())
		if (station->launch_center != nullptr) result.push_back({station->index, *station->launch_center});
	return result;
}

bool ValidateLaunchCenterRecords(std::span<const LaunchCenterRecord> records, uint64_t next, uint32_t width, uint32_t height)
{
	if (records.size() > MAX_LAUNCH_CENTERS || next == 0 || width < 64 || height < 64 || width > 4096 || height > 4096) return false;
	std::set<StationID> stations;
	std::set<uint64_t> generations;
	std::set<TileIndex> occupied;
	for (const auto &record : records) {
		const auto &anchor = record.anchor;
		if (record.station >= StationID::End() || anchor.generation == 0 || anchor.generation >= next || anchor.rotation >= 4 ||
			anchor.tile.base() >= uint64_t{width} * height || !stations.insert(record.station).second || !generations.insert(anchor.generation).second) return false;
		const uint32_t w = anchor.rotation % 2 == 0 ? 4 : 3, h = anchor.rotation % 2 == 0 ? 3 : 4;
		if (anchor.tile.base() % width + w > width || anchor.tile.base() / width + h > height) return false;
		for (uint32_t dy = 0; dy < h; ++dy) for (uint32_t dx = 0; dx < w; ++dx)
			if (!occupied.insert(TileIndex{anchor.tile.base() + dy * width + dx}).second) return false;
	}
	return true;
}

static bool ValidatePhysicalLaunchCenters(std::span<const LaunchCenterRecord> records)
{
	if (!records.empty() && GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return false;
	std::set<StationID> found;
	for (const auto &record : records) {
		const auto *station = Station::GetIfValid(record.station);
		const auto area = GetLaunchCenterFootprint(record.anchor.tile, record.anchor.rotation);
		if (station == nullptr || !Company::IsValidID(station->owner) || !station->facilities.Test(StationFacility::LaunchCenter) ||
			!area || !HasLaunchCenterWaterAccess(*area)) return false;
		for (const auto tile : *area) {
			if (!IsTileType(tile, TileType::Station) || GetStationType(tile) != StationType::LaunchCenter ||
				GetStationIndex(tile) != station->index || GetTileOwner(tile) != station->owner || !IsTileFlat(tile) ||
				GetStationGfx(tile) != record.anchor.rotation || IsBridgeAbove(tile)) return false;
		}
		found.insert(station->index);
	}
	for (const auto *station : Station::Iterate())
		if (station->facilities.Test(StationFacility::LaunchCenter) != found.contains(station->index)) return false;
	// Once per load/save, rather than per tick: reject orphan or extra facility tiles.
	for (const auto tile : Map::Iterate()) {
		if (!IsTileType(tile, TileType::Station) || GetStationType(tile) != StationType::LaunchCenter) continue;
		const auto it = std::ranges::find(records, GetStationIndex(tile), &LaunchCenterRecord::station);
		if (it == records.end()) return false;
		const auto area = GetLaunchCenterFootprint(it->anchor.tile, it->anchor.rotation);
		if (!area || !area->Contains(tile)) return false;
	}
	return true;
}

bool ValidateLaunchCenters()
{
	const auto records = CaptureLaunchCenters();
	if (!ValidateLaunchCenterRecords(records, _next_launch_center_generation, Map::SizeX(), Map::SizeY()) ||
		!ValidatePhysicalLaunchCenters(records) || records.size() != _launch_centers.size()) return false;
	for (const auto &ref : _launch_centers) if (ResolveLaunchCenter(ref) == nullptr) return false;
	return true;
}

bool InstallLoadedLaunchCenters(std::vector<LaunchCenterRecord> records, uint64_t next)
{
	if (!ValidateLaunchCenterRecords(records, next, Map::SizeX(), Map::SizeY()) || !ValidatePhysicalLaunchCenters(records)) return false;
	_launch_centers.clear();
	for (const auto &record : records) {
		auto *station = Station::Get(record.station);
		station->launch_center = std::make_unique<LaunchCenterAnchor>(record.anchor);
		station->launch_hub = std::make_unique<LaunchCollectionHub>();
		_launch_centers.push_back(GetLaunchCenterReference(*station));
	}
	std::ranges::sort(_launch_centers, {}, [](const auto &ref) { return ref.station.base(); });
	_next_launch_center_generation = next;
	return true;
}

uint64_t ComputeLaunchCenterDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	auto hash = [&](uint64_t value) { for (uint shift = 0; shift < 64; shift += 8) { digest ^= (value >> shift) & 0xFF; digest *= 1099511628211ULL; } };
	hash(_next_launch_center_generation);
	for (const auto &record : CaptureLaunchCenters()) {
		hash(record.station.base()); hash(Station::Get(record.station)->owner.base());
		hash(record.anchor.generation); hash(record.anchor.tile.base()); hash(record.anchor.rotation);
	}
	return digest;
}

bool CanRemoveLaunchCenterWater(const std::function<bool(TileIndex)> &removed)
{
	for (const auto ref : _launch_centers) {
		const auto *station = ResolveLaunchCenter(ref);
		if (station == nullptr) return false;
		const auto area = GetLaunchCenterFootprint(station->launch_center->tile, station->launch_center->rotation);
		if (!area) return false;
		bool water_left = false;
		const int x = TileX(area->tile), y = TilePlaneY(area->tile);
		auto inspect = [&](int nx, int ny) {
			if (nx < 0 || ny < 0 || nx >= static_cast<int>(Map::SizeX()) || ny >= static_cast<int>(Map::SizeY())) return;
			const auto tile = TileXYL(nx, ny, MapLayer::Surface);
			if ((tile == _launch_center_replaced_water || !removed(tile)) && IsLaunchCenterNavigableWater(tile)) water_left = true;
		};
		for (uint dx = 0; dx < area->w; ++dx) { inspect(x + dx, y - 1); inspect(x + dx, y + area->h); }
		for (uint dy = 0; dy < area->h; ++dy) { inspect(x - 1, y + dy); inspect(x + area->w, y + dy); }
		if (!water_left) return false;
	}
	return true;
}
