/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file launch_hub.cpp Explicit native Surface freight relationships, independent of catchment. */
#include "stdafx.h"
#include "launch_center.h"
#include "launch_center_cmd.h"
#include "command_func.h"
#include "station_base.h"
#include "station_map.h"
#include "company_base.h"
#include "company_func.h"
#include "map_layer.h"
#include "window_func.h"
#include "viewport_func.h"
#include "gfx_func.h"
#include "table/strings.h"
#include <set>
#include "safeguards.h"

static uint64_t _next_launch_hub_member_generation = 1;

bool IsLaunchHubEligible(const Station &center, const Station &member)
{
	if (center.launch_center == nullptr || center.launch_hub == nullptr || !Company::IsValidID(center.owner) || center.owner != member.owner ||
		!member.facilities.Any({StationFacility::Train, StationFacility::TruckStop}) ||
		member.spread.IsEmpty() || !MultiLayerMap::IsTileInLayer(member.spread.tile, MapLayer::Surface) ||
		uint64_t{TileX(member.spread.tile)} + member.spread.w > Map::SizeX() ||
		uint64_t{TilePlaneY(member.spread.tile)} + member.spread.h > Map::SizeY() ||
		member.spread.w > 2 * LAUNCH_CENTER_COLLECTION_RADIUS + 1 || member.spread.h > 2 * LAUNCH_CENTER_COLLECTION_RADIUS + 1) return false;
	const auto anchor = GetLaunchCenterPlaneReference(center.launch_center->tile);
	if (!anchor) return false;
	bool freight_tile = false;
	for (const auto tile : member.spread) {
		if (!IsTileType(tile, TileType::Station) || GetStationIndex(tile) != member.index) continue;
		const auto point = GetLaunchCenterPlaneReference(tile);
		if (!point || GetTileOwner(tile) != member.owner || !IsInsideLaunchCenterCollectionArea(*point, *anchor)) return false;
		freight_tile |= IsRailStationTile(tile) || IsTruckStop(tile);
	}
	return freight_tile;
}

std::optional<LaunchHubMembership> GetLaunchHubMembership(StationID member_id)
{
	const auto *member = Station::GetIfValid(member_id);
	if (member == nullptr) return std::nullopt;
	if (member->launch_center != nullptr) {
		if (!IsLaunchHubEligible(*member, *member)) return std::nullopt;
		return LaunchHubMembership{GetLaunchCenterReference(*member), {member_id, member->launch_center->generation}, true};
	}
	// At most64 centers x32 explicit memberships; no scan of the native Station pool or map.
	for (const auto &center_record : GetLaunchCenterReferences()) {
		const auto *center = Station::Get(center_record.station);
		if (center->launch_hub == nullptr) continue;
		const auto it = std::ranges::find(center->launch_hub->members, member_id, &LaunchHubMemberRef::station);
		if (it != center->launch_hub->members.end() && IsLaunchHubEligible(*center, *member))
			return LaunchHubMembership{GetLaunchCenterReference(*center), *it, false};
	}
	return std::nullopt;
}

Station *ResolveLaunchHubMember(LaunchCenterRef center, LaunchHubMemberRef member)
{
	if (ResolveLaunchCenter(center) == nullptr) return nullptr;
	const auto membership = GetLaunchHubMembership(member.station);
	return membership && membership->center == center && membership->member == member ? Station::Get(member.station) : nullptr;
}

std::vector<LaunchCenterRef> GetEligibleLaunchHubs(const Station &member)
{
	std::vector<LaunchCenterRef> result;
	for (const auto &record : GetLaunchCenterReferences()) {
		const auto *center = Station::Get(record.station);
		if (member.launch_center != nullptr && member.index != center->index) continue;
		if (IsLaunchHubEligible(*center, member)) result.push_back(GetLaunchCenterReference(*center));
	}
	return result;
}

Station *GetLaunchHubStorage(LaunchCenterRef center) { return ResolveLaunchCenter(center); }
uint64_t GetNextLaunchHubMemberGeneration() { return _next_launch_hub_member_generation; }

std::vector<LaunchHubRecord> CaptureLaunchHubs()
{
	std::vector<LaunchHubRecord> result;
	for (const auto &record : GetLaunchCenterReferences()) {
		const auto *center = Station::Get(record.station);
		if (center->launch_hub != nullptr && !center->launch_hub->members.empty())
			result.push_back({GetLaunchCenterReference(*center), center->launch_hub->members});
	}
	return result;
}

bool ValidateLaunchHubRecords(std::span<const LaunchHubRecord> records, uint64_t next)
{
	if (next == 0 || records.size() > MAX_LAUNCH_CENTERS) return false;
	std::set<StationID> centers, members;
	std::set<uint64_t> generations;
	for (const auto &record : records) {
		if (record.center.station >= StationID::End() || record.center.generation == 0 || !centers.insert(record.center.station).second ||
			record.members.empty() || record.members.size() > MAX_LAUNCH_HUB_MEMBERS) return false;
		StationID previous = StationID::Invalid();
		for (const auto member : record.members) {
			if (member.station >= StationID::End() || member.station == record.center.station || member.generation == 0 || member.generation >= next ||
				!members.insert(member.station).second || !generations.insert(member.generation).second ||
				(previous != StationID::Invalid() && member.station <= previous)) return false;
			previous = member.station;
		}
	}
	// A launch center can never become a separate member of another hub.
	for (const auto station : centers) if (members.contains(station)) return false;
	return true;
}

static bool ValidatePhysicalLaunchHubs(std::span<const LaunchHubRecord> records)
{
	for (const auto &record : records) {
		const auto *center = ResolveLaunchCenter(record.center);
		if (center == nullptr || center->launch_hub == nullptr) return false;
		for (const auto ref : record.members) {
			auto *member = Station::GetIfValid(ref.station);
			if (member == nullptr || member->launch_center != nullptr || !IsLaunchHubEligible(*center, *member)) return false;
		}
	}
	return true;
}

bool ValidateLaunchHubs()
{
	for (const auto *station : Station::Iterate())
		if ((station->launch_center != nullptr) != (station->launch_hub != nullptr)) return false;
	const auto records = CaptureLaunchHubs();
	return ValidateLaunchHubRecords(records, _next_launch_hub_member_generation) && ValidatePhysicalLaunchHubs(records);
}

bool InstallLoadedLaunchHubs(std::vector<LaunchHubRecord> records, uint64_t next)
{
	if (!ValidateLaunchHubRecords(records, next) || !ValidatePhysicalLaunchHubs(records)) return false;
	for (const auto &center : GetLaunchCenterReferences()) Station::Get(center.station)->launch_hub->members.clear();
	for (auto &record : records) ResolveLaunchCenter(record.center)->launch_hub->members = std::move(record.members);
	_next_launch_hub_member_generation = next;
	return true;
}

uint64_t ComputeLaunchHubDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	auto hash = [&](uint64_t value) { for (uint shift = 0; shift < 64; shift += 8) { digest ^= (value >> shift) & 0xFF; digest *= 1099511628211ULL; } };
	hash(_next_launch_hub_member_generation);
	for (const auto &record : CaptureLaunchHubs()) {
		hash(record.center.station.base()); hash(record.center.generation); hash(record.members.size());
		for (const auto member : record.members) { hash(member.station.base()); hash(member.generation); }
	}
	return digest;
}

void ResetLaunchHubs()
{
	ResetSurfaceHandling();
	for (const auto ref : GetLaunchCenterReferences()) SetViewportLaunchCollection(ref.station, ref.generation, false);
	for (auto *station : Station::Iterate()) station->launch_hub.reset();
	_next_launch_hub_member_generation = 1;
}

/** Membership changes retain all native packets at their existing Station owner. */
void ForgetLaunchHubStation(Station &station)
{
	SettleSurfaceHandlingForStation(station, Station::CleaningPool());
	bool changed = station.launch_hub != nullptr;
	for (const auto &record : GetLaunchCenterReferences()) {
		auto *center = Station::Get(record.station);
		if (center->launch_hub == nullptr) continue;
		if (std::erase_if(center->launch_hub->members, [&](const auto member) { return member.station == station.index; }) != 0) {
			SetWindowDirty(WindowClass::StationView, center->index);
			changed = true;
		}
	}
	if (station.launch_hub != nullptr) {
		const auto ref = GetLaunchCenterReference(station);
		SetViewportLaunchCollection(ref.station, ref.generation, false);
		for (const auto member : station.launch_hub->members) SetWindowDirty(WindowClass::StationView, member.station);
		station.launch_hub.reset();
	}
	if (changed) MarkWholeScreenDirty();
}

void RevalidateLaunchHubMemberships()
{
	bool changed = false;
	for (const auto &record : GetLaunchCenterReferences()) {
		auto *center = Station::Get(record.station);
		if (center->launch_hub == nullptr) continue;
		std::erase_if(center->launch_hub->members, [&](const auto ref) {
			auto *member = Station::GetIfValid(ref.station);
			if (member != nullptr && member->launch_center == nullptr && IsLaunchHubEligible(*center, *member)) return false;
			if (member != nullptr) SettleSurfaceHandlingForStation(*member);
			SetWindowDirty(WindowClass::StationView, ref.station); SetWindowDirty(WindowClass::StationView, center->index);
			changed = true;
			return true;
		});
	}
	if (changed) MarkWholeScreenDirty();
}

CommandCost CmdSetLaunchHub(DoCommandFlags flags, StationID member_id, StationID center_id, uint64_t generation)
{
	auto *member = Station::GetIfValid(member_id);
	if (member == nullptr || !Company::IsValidID(member->owner)) return CMD_ERROR;
	const auto owned = CheckOwnership(member->owner);
	if (owned.Failed()) return owned;
	if (center_id == StationID::Invalid() && generation != 0) return CMD_ERROR;
	auto *center = center_id == StationID::Invalid() ? nullptr : ResolveLaunchCenter({center_id, generation});
	if (center_id != StationID::Invalid() && (center == nullptr || !IsLaunchHubEligible(*center, *member)))
		return CommandCost(STR_ERROR_LAUNCH_HUB_INELIGIBLE);
	if (member->launch_center != nullptr) {
		return center == member ? CommandCost() : CommandCost(STR_ERROR_LAUNCH_HUB_JOINED);
	}
	const auto old = GetLaunchHubMembership(member_id);
	if (old && old->center == LaunchCenterRef{center_id, generation}) return CommandCost();
	// A deliberate unlink/reselection waits for its current physical handling to settle.
	if (HasSurfaceHandlingForStation(member_id)) return CommandCost(STR_ERROR_LAUNCH_HANDLING_ACTIVE);
	if (center != nullptr && (center->launch_hub->members.size() >= MAX_LAUNCH_HUB_MEMBERS || _next_launch_hub_member_generation == UINT64_MAX))
		return CommandCost(STR_ERROR_LAUNCH_HUB_LIMIT);
	if (flags.Test(DoCommandFlag::Execute)) {
		ForgetLaunchHubStation(*member);
		if (center != nullptr) {
			center->launch_hub->members.push_back({member_id, _next_launch_hub_member_generation++});
			std::ranges::sort(center->launch_hub->members, {}, [](const auto &ref) { return ref.station.base(); });
			SetWindowDirty(WindowClass::StationView, center_id);
		}
		SetWindowDirty(WindowClass::StationView, member_id);
	}
	return CommandCost();
}
