/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_handling.cpp Saved, bounded launch-hub cargo ownership and lifecycle. */
#include "stdafx.h"
#include "launch_center.h"
#include "launch_center_cmd.h"
#include "hub_cargo.h"
#include "multilayer_space_content.h"
#include "station_base.h"
#include "vehicle_base.h"
#include "company_base.h"
#include "company_func.h"
#include "command_func.h"
#include "cargotype.h"
#include "window_func.h"
#include "timer/timer_game_tick.h"
#include <set>
#include <map>
#include "safeguards.h"

static uint64_t _next_surface_handling_id = 1;

uint64_t GetNextSurfaceHandlingID() { return _next_surface_handling_id; }

bool IsLaunchSupplyCargo(CargoType cargo)
{
	if (cargo >= NUM_CARGO || !CargoSpec::Get(cargo)->IsValid()) return false;
	static constexpr std::array supplies{CargoLabel{"STEL"}, CargoLabel{"GOOD"}, CargoLabel{"PROP"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}, CargoLabel{"SCMP"}, CargoLabel{"RBPT"}, CargoLabel{"SMIK"}};
	return std::ranges::find(supplies, CargoSpec::Get(cargo)->label) != supplies.end();
}

static uint IncomingHandling(const Station &target, CargoType cargo)
{
	uint count = 0;
	for (const auto ref : GetLaunchCenterReferences()) {
		const auto *center = ResolveLaunchCenter(ref);
		for (const auto &job : center->launch_hub->handling) {
			if (job->cargo == cargo && (job->reverse ? ref.station : job->member.station) == target.index) count += job->packets.AvailableCount();
		}
	}
	return count;
}
uint GetLaunchStockCapacity(const Station &station, CargoType cargo)
{
	if (!IsValidCargoType(cargo)) return 0;
	if (!station.launch_center) return LAUNCH_HANDLING_STOCK_CAP;
	const auto &b = GetMultiLayerSpaceBudget(); const auto label = CargoSpec::Get(cargo)->label;
	if (label == CargoLabel{"PROP"}) return b.launch_propellant_cap;
	if (label == CargoLabel{"RFMT"}) return b.launch_metal_cap;
	if (label == CargoLabel{"ELEC"}) return b.launch_electronics_cap;
	if (label == CargoLabel{"SCMP"}) return b.launch_components_cap;
	return b.stock_cap;
}
uint GetLaunchStockHeadroom(const Station &station, CargoType cargo)
{
	const auto cap = GetLaunchStockCapacity(station, cargo); if (cap == 0) return 0;
	const uint64_t occupied = uint64_t{station.goods[cargo].TotalCount()} + IncomingHandling(station, cargo) +
		(station.launch_center ? GetIncomingSpaceDescent(GetLaunchCenterReference(station), cargo) : 0);
	return occupied >= cap ? 0 : static_cast<uint>(cap - occupied);
}

bool CanStartSurfaceHandling(LaunchCenterRef ref, LaunchHubMemberRef member_ref, CargoType cargo, uint16_t quantity, bool reverse)
{
	const auto *center = ResolveLaunchCenter(ref);
	const auto *member = ResolveLaunchHubMember(ref, member_ref);
	if (center == nullptr || member == nullptr || center == member || !Company::IsValidID(center->owner) ||
		cargo >= NUM_CARGO || !CargoSpec::Get(cargo)->IsValid() || quantity == 0 || quantity > MAX_SURFACE_HANDLING_QUANTITY ||
		(reverse && !IsLaunchSupplyCargo(cargo)) ||
		center->launch_hub->handling.size() >= MAX_SURFACE_HANDLING_JOBS || _next_surface_handling_id == UINT64_MAX) return false;
	const auto *source = reverse ? member : center;
	const auto *target = reverse ? center : member;
	return quantity <= GetLaunchStockHeadroom(*target, cargo) &&
		source->goods[cargo].HasData() && HubCargoTransfer::CanMove(source->goods[cargo].GetData().cargo, quantity);
}

bool StartSurfaceHandling(LaunchCenterRef ref, LaunchHubMemberRef member_ref, CargoType cargo, uint16_t quantity, bool reverse)
{
	if (!CanStartSurfaceHandling(ref, member_ref, cargo, quantity, reverse)) return false;
	auto *center = ResolveLaunchCenter(ref);
	auto *source = reverse ? ResolveLaunchHubMember(ref, member_ref) : center;
	auto job = std::make_unique<SurfaceHandlingJob>();
	job->id = _next_surface_handling_id++;
	job->center = ref; job->member = member_ref; job->cargo = cargo; job->reverse = reverse;
	job->remaining_ticks = (quantity + LAUNCH_HANDLING_THROUGHPUT - 1) / LAUNCH_HANDLING_THROUGHPUT;
	const bool moved = HubCargoTransfer::Move(source->goods[cargo].GetData().cargo, job->packets, quantity);
	assert(moved);
	center->launch_hub->handling.push_back(std::move(job));
	SetWindowDirty(WindowClass::StationView, source->index);
	return true;
}

bool HasSurfaceHandlingForStation(StationID station)
{
	for (const auto ref : GetLaunchCenterReferences()) for (const auto &job : ResolveLaunchCenter(ref)->launch_hub->handling)
		if (ref.station == station || job->member.station == station) return true;
	return false;
}

/** Mutations settle before retiring a link. Even a full station retains returned packets instead of destroying them. */
void SettleSurfaceHandlingForStation(Station &station, bool cleaning_pool)
{
	for (const auto ref : GetLaunchCenterReferences()) {
		auto *center = ResolveLaunchCenter(ref);
		if (center == nullptr || center->launch_hub == nullptr) continue;
		std::erase_if(center->launch_hub->handling, [&](const auto &job) {
			if (ref.station != station.index && job->member.station != station.index) return false;
			if (cleaning_pool) { job->packets.OnCleanPool(); return true; }
			// Removing a member returns its in-flight cargo to the retained center.
			// Removing a center is preflight-refused by the command while jobs exist;
			// direct native destruction evacuates to the exact surviving member.
			// Identity survives an ownership change until the link is retired. Settlement
			// must not require the old commercial eligibility after bankruptcy/transfer.
			Station *retained = center;
			if (center == &station) {
				const bool linked = std::ranges::find(center->launch_hub->members, job->member) != center->launch_hub->members.end();
				retained = linked ? Station::GetIfValid(job->member.station) : nullptr;
			}
			assert(retained != nullptr && retained != &station);
			if (retained == nullptr || retained == &station) return false;
			const bool moved = HubCargoTransfer::Move(job->packets, retained->goods[job->cargo].GetOrCreateData().cargo, job->packets.AvailableCount());
			assert(moved); // Whole packets return; settlement never needs split capacity.
			SetWindowDirty(WindowClass::StationView, retained->index);
			return true;
		});
	}
}

void TickSurfaceHandling()
{
	RevalidateLaunchHubMemberships();
	for (const auto ref : GetLaunchCenterReferences()) {
		auto *center = ResolveLaunchCenter(ref);
		auto &hub = *center->launch_hub;
		uint allowance = LAUNCH_HANDLING_THROUGHPUT;
		for (auto &job : hub.handling) {
			if (++job->age_ticks == Ticks::CARGO_AGING_TICKS) { job->age_ticks = 0; HubCargoTransfer::Age(job->packets); }
			auto *member = ResolveLaunchHubMember(ref, job->member);
			if (member == nullptr || allowance == 0) continue;
			auto *target = job->reverse ? center : member;
			const uint occupied = target->goods[job->cargo].TotalCount();
			const uint cap = GetLaunchStockCapacity(*target, job->cargo);
			const uint quantity = std::min({allowance, job->packets.AvailableCount(), occupied >= cap ? 0U : cap - occupied});
			if (quantity == 0 || !HubCargoTransfer::CanMove(job->packets, quantity)) continue;
			const uint mass = quantity * CargoSpec::Get(job->cargo)->weight + hub.handling_cost_remainder;
			const Money cost = mass / 16;
			if (Company::Get(center->owner)->money < cost) continue;
			const bool moved = HubCargoTransfer::Move(job->packets, target->goods[job->cargo].GetOrCreateData().cargo, quantity);
			assert(moved);
			hub.handling_cost_remainder = mass % 16;
			SubtractMoneyFromCompany(center->owner, CommandCost(ExpensesType::Other, cost));
			allowance -= quantity;
			job->remaining_ticks = (job->packets.AvailableCount() + LAUNCH_HANDLING_THROUGHPUT - 1) / LAUNCH_HANDLING_THROUGHPUT;
			SetWindowDirty(WindowClass::StationView, target->index);
		}
		std::erase_if(hub.handling, [](const auto &job) { return job->packets.AvailableCount() == 0; });
		// Pickup demand acts as the cargo filter. Allocate in native member/cargo order,
		// never by catchment or UI order. Reverse supply handling is always explicit.
		for (const auto member_ref : hub.members) {
			auto *member = ResolveLaunchHubMember(ref, member_ref);
			if (member == nullptr) continue;
			for (CargoType cargo : EnumRange(NUM_CARGO)) {
				if (!member->goods[cargo].HasVehicleEverTriedLoading() || !center->goods[cargo].HasData()) continue;
				const auto &stock = center->goods[cargo].GetData().cargo;
				if (stock.AvailableCount() == 0 || stock.Packets()->begin()->second.front()->GetSource().type != SourceType::Asteroid) continue;
				const uint occupied = member->goods[cargo].TotalCount() + IncomingHandling(*member, cargo);
				if (occupied >= LAUNCH_HANDLING_STOCK_CAP) continue;
				const auto quantity = static_cast<uint16_t>(std::min({stock.AvailableCount(), uint{MAX_SURFACE_HANDLING_QUANTITY}, LAUNCH_HANDLING_STOCK_CAP - occupied}));
				StartSurfaceHandling(ref, member_ref, cargo, quantity, false);
			}
		}
	}
}

std::vector<SurfaceHandlingRecord> CaptureSurfaceHandling()
{
	std::vector<SurfaceHandlingRecord> records;
	for (const auto ref : GetLaunchCenterReferences()) for (const auto &job : ResolveLaunchCenter(ref)->launch_hub->handling) {
		SurfaceHandlingRecord record{job->id, job->center, job->member, job->cargo, job->reverse, job->remaining_ticks, job->age_ticks, {}};
		for (StationCargoList::ConstIterator it(job->packets.Packets()->begin()); it != job->packets.Packets()->end(); ++it) record.packets.emplace_back((*it)->index, it.GetKey());
		records.push_back(std::move(record));
	}
	std::ranges::sort(records, {}, &SurfaceHandlingRecord::id);
	return records;
}

std::vector<SurfaceHandlingCostRecord> CaptureSurfaceHandlingCosts()
{
	std::vector<SurfaceHandlingCostRecord> records;
	for (const auto ref : GetLaunchCenterReferences()) {
		const auto remainder = ResolveLaunchCenter(ref)->launch_hub->handling_cost_remainder;
		if (remainder != 0) records.push_back({ref, remainder});
	}
	return records;
}

bool ValidateSurfaceHandlingRecords(std::span<const SurfaceHandlingRecord> records, std::span<const SurfaceHandlingCostRecord> costs, uint64_t next)
{
	if (next == 0 || records.size() > MAX_LAUNCH_CENTERS * MAX_SURFACE_HANDLING_JOBS || costs.size() > MAX_LAUNCH_CENTERS) return false;
	if ((!records.empty() || !costs.empty()) && next == 1) return false;
	std::set<CargoPacketID> packets;
	std::map<StationID, uint> per_center;
	uint64_t previous = 0;
	for (const auto &record : records) {
		if (record.id <= previous || record.id >= next || record.center.station >= StationID::End() || record.center.generation == 0 ||
			record.member.station >= StationID::End() || record.member.station == record.center.station || record.member.generation == 0 ||
			record.cargo >= NUM_CARGO || record.remaining_ticks == 0 || record.remaining_ticks > MAX_SURFACE_HANDLING_QUANTITY / LAUNCH_HANDLING_THROUGHPUT ||
			record.age_ticks >= Ticks::CARGO_AGING_TICKS || record.packets.empty() || record.packets.size() > MAX_SURFACE_HANDLING_QUANTITY ||
			++per_center[record.center.station] > MAX_SURFACE_HANDLING_JOBS) return false;
		previous = record.id;
		for (const auto &[packet, hop] : record.packets) if (packet >= CargoPacketID::End() || !packets.insert(packet).second || (hop != StationID::Invalid() && hop >= StationID::End())) return false;
	}
	std::set<StationID> cost_centers;
	for (const auto &cost : costs) if (cost.center.station >= StationID::End() || cost.center.generation == 0 || cost.remainder == 0 || cost.remainder >= 16 || !cost_centers.insert(cost.center.station).second) return false;
	return true;
}

static bool ValidatePhysicalHandling(std::span<const SurfaceHandlingRecord> records, std::span<const SurfaceHandlingCostRecord> costs)
{
	std::set<CargoPacketID> owned;
	for (const auto *station : Station::Iterate()) for (const auto &goods : station->goods) {
		if (!goods.HasData()) continue;
		for (const auto &[hop, packets] : *goods.GetData().cargo.Packets()) for (const auto *packet : packets) if (!owned.insert(packet->index).second) return false;
	}
	for (const auto *vehicle : Vehicle::Iterate()) for (const auto *packet : *vehicle->cargo.Packets()) if (!owned.insert(packet->index).second) return false;
	for (const auto &record : records) {
		if (ResolveLaunchHubMember(record.center, record.member) == nullptr || !CargoSpec::Get(record.cargo)->IsValid() || (record.reverse && !IsLaunchSupplyCargo(record.cargo))) return false;
		uint quantity = 0;
		for (const auto &[id, hop] : record.packets) {
			const auto *packet = CargoPacket::GetIfValid(id);
			if (packet == nullptr || packet->Count() == 0 || !owned.insert(id).second) return false;
			quantity += packet->Count();
		}
		if (quantity > MAX_SURFACE_HANDLING_QUANTITY || record.remaining_ticks != (quantity + LAUNCH_HANDLING_THROUGHPUT - 1) / LAUNCH_HANDLING_THROUGHPUT) return false;
	}
	for (const auto &cost : costs) if (ResolveLaunchCenter(cost.center) == nullptr) return false;
	return true;
}

bool ValidateSurfaceHandling()
{
	for (const auto ref : GetLaunchCenterReferences()) for (const auto &job : ResolveLaunchCenter(ref)->launch_hub->handling)
		if (job->center != ref) return false;
	const auto records = CaptureSurfaceHandling();
	const auto costs = CaptureSurfaceHandlingCosts();
	return ValidateSurfaceHandlingRecords(records, costs, _next_surface_handling_id) && ValidatePhysicalHandling(records, costs);
}

bool InstallLoadedSurfaceHandling(std::span<const SurfaceHandlingRecord> records, std::span<const SurfaceHandlingCostRecord> costs, uint64_t next)
{
	if (!ValidateSurfaceHandlingRecords(records, costs, next) || !ValidatePhysicalHandling(records, costs)) return false;
	for (const auto ref : GetLaunchCenterReferences()) if (!ResolveLaunchCenter(ref)->launch_hub->handling.empty()) return false;
	// Validate the complete transport before any packet is installed in its new owner.
	for (const auto &record : records) {
		auto job = std::make_unique<SurfaceHandlingJob>();
		job->id = record.id; job->center = record.center; job->member = record.member; job->cargo = record.cargo; job->reverse = record.reverse;
		job->remaining_ticks = record.remaining_ticks; job->age_ticks = record.age_ticks;
		for (const auto &[id, hop] : record.packets) job->packets.Append(CargoPacket::Get(id), hop);
		ResolveLaunchCenter(record.center)->launch_hub->handling.push_back(std::move(job));
	}
	for (const auto &cost : costs) ResolveLaunchCenter(cost.center)->launch_hub->handling_cost_remainder = cost.remainder;
	_next_surface_handling_id = next;
	return true;
}

void ResetSurfaceHandling()
{
	for (const auto ref : GetLaunchCenterReferences()) {
		auto *center = ResolveLaunchCenter(ref);
		if (center == nullptr || center->launch_hub == nullptr) continue;
		center->launch_hub->handling.clear(); center->launch_hub->handling_cost_remainder = 0;
	}
	_next_surface_handling_id = 1;
}

uint64_t ComputeSurfaceHandlingDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	auto hash = [&](uint64_t value) { for (uint shift = 0; shift < 64; shift += 8) { digest ^= (value >> shift) & 0xFF; digest *= 1099511628211ULL; } };
	hash(_next_surface_handling_id);
	for (const auto &record : CaptureSurfaceHandling()) {
		hash(record.id); hash(record.center.station.base()); hash(record.center.generation); hash(record.member.station.base()); hash(record.member.generation);
		hash(to_underlying(record.cargo)); hash(record.reverse); hash(record.remaining_ticks); hash(record.age_ticks);
		for (const auto &[id, hop] : record.packets) { const auto *packet = CargoPacket::Get(id); hash(id.base()); hash(hop.base()); hash(packet->Count()); hash(packet->GetPeriodsInTransit()); hash(static_cast<int64_t>(packet->GetFeederShare())); }
	}
	for (const auto &cost : CaptureSurfaceHandlingCosts()) { hash(cost.center.station.base()); hash(cost.center.generation); hash(cost.remainder); }
	return digest;
}

CommandCost CmdStartSurfaceHandling(DoCommandFlags flags, StationID member_id, uint64_t member_generation, StationID center_id, uint64_t generation, CargoType cargo, uint16_t quantity, bool reverse)
{
	const LaunchCenterRef center_ref{center_id, generation};
	const auto *center = ResolveLaunchCenter(center_ref);
	const auto membership = GetLaunchHubMembership(member_id);
	if (center == nullptr || !membership || membership->center != center_ref || membership->member.generation != member_generation) return CMD_ERROR;
	const auto owned = CheckOwnership(center->owner);
	if (owned.Failed()) return owned;
	if (!CanStartSurfaceHandling(center_ref, membership->member, cargo, quantity, reverse)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		const bool started = StartSurfaceHandling(center_ref, membership->member, cargo, quantity, reverse);
		assert(started);
	}
	return CommandCost(); // Fees accrue on the actual timed transfer, once, in the native owner account.
}
