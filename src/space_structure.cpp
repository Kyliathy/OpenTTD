/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure.cpp Native paid sites, Core activation and exclusive orbital cargo owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-6.
 */
#include "stdafx.h"
#include "space_structure.h"
#include "space_structure_cmd.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "orbital_service.h"
#include "space_pod.h"
#include "company_base.h"
#include "company_func.h"
#include "station_base.h"
#include "hub_cargo.h"
#include "command_func.h"
#include "map_func.h"
#include "map_layer.h"
#include "gfx_func.h"
#include "core/pool_func.hpp"
#include "timer/timer_game_tick.h"
#include <bit>
#include <set>
#include "safeguards.h"

SpaceStructurePool _space_structure_pool("SpaceStructure");
INSTANTIATE_POOL_METHODS(SpaceStructure)
static uint64_t next_generation = 1;
/** @copydoc GetSpaceStructureReference */
SpaceStructureRef GetSpaceStructureReference(const SpaceStructure &site) { return {site.index, site.generation}; }
/** @copydoc ResolveSpaceStructure */
SpaceStructure *ResolveSpaceStructure(SpaceStructureRef ref)
{
	auto *site = SpaceStructure::GetIfValid(ref.structure);
	return site != nullptr && ref.generation != 0 && site->generation == ref.generation ? site : nullptr;
}
/** @copydoc GetNextSpaceStructureGeneration */
uint64_t GetNextSpaceStructureGeneration() { return next_generation; }
/** @copydoc ResetSpaceStructures */
void ResetSpaceStructures() { next_generation = 1; ResetSpaceStructureLoadStaging(); ResetOrbitalConstructionTick(); }
/** @copydoc SpaceStructure::~SpaceStructure */
SpaceStructure::~SpaceStructure()
{
	// World teardown has already cleaned the packet pool; normal removal must never discard accountable material.
	if (SpaceStructure::CleaningPool()) for (auto &slot : this->job.inputs) slot.OnCleanPool();
	else for (const auto &slot : this->job.inputs) assert(slot.TotalCount() == 0);
	if (this->module) {
		// Completed inventory follows the same native teardown/removal rule as construction inventory.
		for (auto &slot : this->module->stock) {
			if (SpaceStructure::CleaningPool()) slot.OnCleanPool();
			else assert(slot.TotalCount() == 0);
		}
		// Service reservations remain actual accountable material even after their craft is removed.
		for (auto &slot : this->module->service.inputs) {
			if (SpaceStructure::CleaningPool()) slot.OnCleanPool();
			else assert(slot.TotalCount() == 0);
		}
	}
}
/** @copydoc GetSpaceStructureApproach */
SpacePoint GetSpaceStructureApproach(const SpaceStructureState &s)
{
	const auto *d = GetSpaceConstructionDefinition(s.kind);
	return {int32_t{s.x} * SPACE_Q16, (int32_t{s.y} + (d == nullptr ? 0 : d->height)) * SPACE_Q16};
}
/** @copydoc IsSpaceStructureSiteValid */
bool IsSpaceStructureSiteValid(const SpaceStructureState &s, uint32_t width, uint32_t height)
{
	const auto *d = GetSpaceConstructionDefinition(s.kind);
	// The full static footprint and south approach must fit a supported plane, without entering moving-body space.
	if (width < 64 || height < 64 || width > 4096 || height > 4096 || !std::has_single_bit(width) || !std::has_single_bit(height) ||
		d == nullptr || d->width == 0 || d->height == 0 || uint32_t{s.x} + d->width > width || uint32_t{s.y} + d->height >= height) return false;
	return uint32_t{s.y} + d->height < height / 4 || s.y >= height * 3 / 4;
}
/** @copydoc SpaceStructureSitesOverlap */
bool SpaceStructureSitesOverlap(const SpaceStructureState &a, const SpaceStructureState &b)
{
	const auto *ad = GetSpaceConstructionDefinition(a.kind), *bd = GetSpaceConstructionDefinition(b.kind);
	// Unknown definitions cannot reserve a safe footprint or grant overlap permission.
	if (ad == nullptr || bd == nullptr) return true;
	/** @param s Site geometry. @param d Canonical footprint. @param x Candidate X. @param y Candidate Y. @return Whether its occupied cells include the point. */
	auto contains = [](const SpaceStructureState &s, const SpaceConstructionDefinition &d, uint x, uint y) {
		return (x >= s.x && x < uint{s.x} + d.width && y >= s.y && y < uint{s.y} + d.height) || (x == s.x && y == uint{s.y} + d.height);
	};
	const bool footprints = a.x < uint{b.x} + bd->width && b.x < uint{a.x} + ad->width && a.y < uint{b.y} + bd->height && b.y < uint{a.y} + ad->height;
	return footprints || contains(a, *ad, b.x, uint{b.y} + bd->height) || contains(b, *bd, a.x, uint{a.y} + ad->height);
}
/** A cancelled job keeps its site until all its unconsumed cargo is collected.
 * @param site Actual site. @return Whether its footprint remains reserved.
 */
static bool ReservesSite(const SpaceStructure &site)
{
	return site.state.phase != SpaceStructurePhase::Cancelled || site.job.inputs[0].TotalCount() != 0 || site.job.inputs[1].TotalCount() != 0;
}
/** @copydoc IsSpaceStructureSiteAvailable */
bool IsSpaceStructureSiteAvailable(const SpaceStructureState &s)
{
	if (!IsSpaceStructureSiteValid(s, Map::SizeX(), Map::SizeY())) return false;
	// Retained cancelled cargo still occupies the physical site until explicitly collected.
	for (const auto *site : SpaceStructure::Iterate()) if (ReservesSite(*site) && SpaceStructureSitesOverlap(s, site->state)) return false;
	return true;
}

/** @copydoc CmdBuildSpaceBlueprint */
CommandCost CmdBuildSpaceBlueprint(DoCommandFlags flags, TileIndex tile, SpaceBuildKind kind)
{
	const auto *definition = GetSpaceConstructionDefinition(kind);
	// Mobile/carried recipes and unknown kinds cannot create native static occupancy.
	if (definition == nullptr || definition->width == 0 || definition->height == 0) return CMD_ERROR;
	const auto &d = *definition;
	// Placement requires an existing company, explicit Space layer, introduction date and bounded native identity capacity.
	if (!Company::IsValidID(_current_company) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		!MultiLayerMap::IsTileInLayer(tile, MapLayer::Space) || TimerGameCalendar::year < d.introduction_year || next_generation == UINT64_MAX || !SpaceStructure::CanAllocateItem()) return CMD_ERROR;
	SpaceStructureState s; s.owner = _current_company; s.kind = kind; s.x = static_cast<uint16_t>(TileX(tile)); s.y = static_cast<uint16_t>(TilePlaneY(tile)); s.capital_paid = d.capital;
	if (!IsSpaceStructureSiteAvailable(s)) return CMD_ERROR;
	// The first Core bootstraps independently; every extension needs a real operational owned anchor.
	if (kind != SpaceBuildKind::Core && GetOrbitalConstructionCore(s) == nullptr) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) { auto *site = SpaceStructure::Create(); site->generation = next_generation++; site->state = s; MarkWholeScreenDirty(); }
	return CommandCost(ExpensesType::Construction, d.capital);
}
/** @copydoc CmdCancelSpaceBlueprint */
CommandCost CmdCancelSpaceBlueprint(DoCommandFlags flags, SpaceStructureID id, uint64_t generation)
{
	auto *site = ResolveSpaceStructure({id, generation});
	// Only the exact owned inert job can be cancelled; completed facility stock cannot be discarded this way.
	if (site == nullptr || site->state.owner != _current_company || !Company::IsValidID(_current_company) || site->state.phase != SpaceStructurePhase::Blueprint) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) { site->state.phase = SpaceStructurePhase::Cancelled; MarkWholeScreenDirty(); }
	return CommandCost{};
}
/** @copydoc CmdCollectSpaceConstructionInputs */
CommandCost CmdCollectSpaceConstructionInputs(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, VehicleID vehicle, uint64_t vehicle_generation, CargoType cargo, uint16_t quantity)
{
	auto *site = ResolveSpaceStructure({id, generation}); auto *probe = ResolveSpaceCraft({vehicle, vehicle_generation});
	// Cancellation returns only real unconsumed inputs to a compatible owned builder at the exact physical approach.
	if (site == nullptr || probe == nullptr || !Company::IsValidID(_current_company) || site->state.owner != _current_company || probe->owner != _current_company ||
		site->state.phase != SpaceStructurePhase::Cancelled || GetSpaceVehicleModel(*probe) != 20 || probe->construction.placement != SpacePlacement::Orbital || probe->vehstatus.Test(VehState::Crashed) ||
		probe->position != GetSpaceStructureApproach(site->state) || (probe->manifest && probe->manifest->service_ticks != 0) ||
		quantity == 0 || !IsValidCargoType(cargo) || (probe->cargo.TotalCount() != 0 && probe->cargo_type != cargo)) return CMD_ERROR;
	const auto &d = *GetSpaceConstructionDefinition(site->state.kind);
	// Match by the declared physical label; a different cargo cannot stand in for missing construction material.
	size_t slot = 0; while (slot < d.inputs.size() && GetCargoTypeByLabel(d.inputs[slot].label) != cargo) ++slot;
	// Hold space and packet allocation are preflighted before any inventory leaves the retained job.
	if (slot == d.inputs.size() || probe->cargo.TotalCount() + quantity > GetSpaceFreightCapacity(20, cargo) || !HubCargoTransfer::CanMove(site->job.inputs[slot], quantity)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		const bool moved = HubCargoTransfer::Move(site->job.inputs[slot], probe->cargo, quantity, probe->tile); assert(moved);
		if (!moved) return CMD_ERROR;
		probe->cargo_type = cargo; probe->cargo_cap = GetSpaceFreightCapacity(20, cargo); probe->MarkDirty(); MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc CmdActivateOrbitalCore */
CommandCost CmdActivateOrbitalCore(DoCommandFlags flags, SpaceStructureID id, uint64_t generation)
{
	auto *site = ResolveSpaceStructure({id, generation});
	// An explicit, once-only activation follows the complete paid recipe; old saves and previews cannot grant it implicitly.
	if (site == nullptr || site->state.owner != _current_company || !Company::IsValidID(_current_company) ||
		site->state.kind != SpaceBuildKind::Core || site->state.phase != SpaceStructurePhase::FrameComplete || site->module != nullptr) return CMD_ERROR;
	const auto source = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::OrbitalPanel, flags.Test(DoCommandFlag::Execute));
	if (!source) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
		site->module = std::make_unique<OrbitalModule>();
		site->module->producer = *source;
		site->module->activation_ticks = GetMultiLayerSpaceBudget().core_activation_ticks;
		MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc CmdMoveOrbitalFreight */
CommandCost CmdMoveOrbitalFreight(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, VehicleID vehicle, uint64_t vehicle_generation, CargoType cargo, uint16_t quantity, bool load)
{
	auto *site = ResolveSpaceStructure({id, generation});
	auto *craft = ResolveSpaceCraft({vehicle, vehicle_generation});
	// Physical exchange requires the exact owned operational module and an idle orbital craft at its precise approach.
	if (site == nullptr || craft == nullptr || site->state.owner != _current_company || craft->owner != _current_company ||
		!IsOrbitalModuleOperational(*site) || craft->construction.placement != SpacePlacement::Orbital ||
		craft->position != GetSpaceStructureApproach(site->state) || craft->vehstatus.Test(VehState::Crashed) ||
		(craft->manifest && craft->manifest->service_ticks != 0) || quantity == 0 || !IsValidCargoType(cargo) ||
		(craft->cargo.TotalCount() != 0 && craft->cargo_type != cargo)) return CMD_ERROR;
	// An occupied service berth cannot simultaneously move freight or lend its craft to another physical operation.
	if (site->module->service.remaining_ticks != 0 || HasOrbitalServiceAssignment(*craft)) return CMD_ERROR;
	auto &stock = site->module->stock[cargo];
	const auto model = GetSpaceVehicleModel(*craft);
	const auto capacity = GetSpaceFreightCapacity(model, cargo);
	if (load) {
		const auto label = CargoSpec::Get(cargo)->label;
		// Builders load the same two common materials as at Earth commissioning. Mining probes unload their extraction;
		// loading construction stock into a miner would create a fleet state its native provenance/role validator refuses.
		if (model != 20 || (label != CargoLabel{"STEL"} && label != CargoLabel{"GOOD"})) return CMD_ERROR;
		// Probe freight uses its own mass-bounded hold, separate from paid service/pod compartments.
		if (craft->cargo.TotalCount() + quantity > capacity || !HubCargoTransfer::CanMove(stock, quantity)) return CMD_ERROR;
	} else {
		// Whole destination admission preserves the freight hold when module stock is full or packets cannot move.
		if (stock.TotalCount() + quantity > GetOrbitalModuleCargoCapacity(*site, cargo) || !HubCargoTransfer::CanMove(craft->cargo, quantity)) return CMD_ERROR;
		size_t packets = craft->cargo.Packets()->size();
		// Conservatively preflight transport record capacity before a move; packet merging may use fewer slots, never more.
		for (const auto &[hop, list] : *stock.Packets()) { (void)hop; packets += list.size(); }
		if (packets > 64) return CMD_ERROR;
	}
	if (flags.Test(DoCommandFlag::Execute)) {
		const bool moved = load ? HubCargoTransfer::Move(stock, craft->cargo, quantity, craft->tile) : HubCargoTransfer::Move(craft->cargo, stock, quantity, craft->tile);
		assert(moved);
		if (!moved) return CMD_ERROR;
		craft->cargo_type = cargo; craft->cargo_cap = capacity; craft->MarkDirty(); MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc ChangeSpaceStructureOwnership */
void ChangeSpaceStructureOwnership(Owner old_owner, Owner new_owner)
{
	// Takeovers transfer the actual site; company deletion retains inventory under OWNER_NONE before ID reuse.
	for (auto *site : SpaceStructure::Iterate()) if (site->state.owner == old_owner) site->state.owner = Company::IsValidID(new_owner) ? new_owner : OWNER_NONE;
}
/** @copydoc CaptureSpaceStructures */
std::vector<SpaceStructureRecord> CaptureSpaceStructures()
{
	std::vector<SpaceStructureRecord> rows;
	// Preserve exact packet identity and next hops in each physical container, rather than manufacturing quantity-only stock.
	for (const auto *site : SpaceStructure::Iterate()) {
		SpaceStructureRecord r; r.ref = GetSpaceStructureReference(*site); r.state = site->state;
		static_cast<OrbitalConstructionState &>(r) = site->job; r.age_ticks = site->job.age_ticks; r.consumed = site->job.consumed;
		// Record both common construction labels independently so their material receipt remains auditable.
		for (uint i = 0; i < 2; ++i) {
			r.quantities[i] = static_cast<uint16_t>(site->job.inputs[i].TotalCount());
			for (const auto &[hop, packets] : *site->job.inputs[i].Packets()) for (const auto *packet : packets) r.inputs[i].push_back({packet->index, hop});
		}
		if (site->module) {
			r.module.emplace(); r.module->activation_ticks = site->module->activation_ticks; r.module->age_ticks = site->module->age_ticks;
			static_cast<OrbitalFacilityState &>(*r.module) = *site->module;
			static_cast<OrbitalServiceState &>(r.module->service) = site->module->service;
			// Reserved service parts remain separate packet owners from both general stock and construction.
			for (uint i = 0; i < 2; ++i) {
				r.module->service.quantities[i] = static_cast<uint16_t>(site->module->service.inputs[i].TotalCount());
				for (const auto &[hop, packets] : *site->module->service.inputs[i].Packets()) for (const auto *packet : packets) r.module->service.inputs[i].push_back({packet->index, hop});
			}
			// Operational cargo has a separate sole owner from the fully consumed construction recipe.
			for (CargoType cargo : EnumRange(NUM_CARGO)) {
				const auto &stock = site->module->stock[cargo]; r.module->quantities[cargo] = static_cast<uint16_t>(stock.TotalCount());
				for (const auto &[hop, packets] : *stock.Packets()) for (const auto *packet : packets) r.module->stock[cargo].push_back({packet->index, hop});
			}
		}
		rows.push_back(std::move(r));
	}
	return rows;
}
/** @copydoc ValidateSpaceStructureRecords */
bool ValidateSpaceStructureRecords(std::span<const SpaceStructureRecord> rows, uint64_t next, uint32_t width, uint32_t height, uint64_t native_tick)
{
	// Bounded issuance and record count prevent identity reuse and unbounded copied transport.
	if (next == 0 || rows.size() > SpaceStructureID::End().base()) return false;
	std::set<SpaceStructureID> ids; std::set<uint64_t> generations; std::set<CargoPacketID> packets;
	std::set<std::pair<VehicleID, uint64_t>> service_craft;
	// Validate every paid site and all its exclusive packet references before any native owner can be installed.
	for (const auto &r : rows) {
		const auto &s = r.state; const auto *d = GetSpaceConstructionDefinition(s.kind);
		// Unknown/mobile recipes have no legal static site or material receipt.
		if (d == nullptr || d->width == 0 || d->height == 0) return false;
		// Identities, paid recipe, full geometry and bounded work must agree; a frame cannot bypass material consumption.
		if (r.ref.structure >= SpaceStructureID::End() || r.ref.generation == 0 || r.ref.generation >= next || !ids.insert(r.ref.structure).second || !generations.insert(r.ref.generation).second ||
			(s.owner != OWNER_NONE && s.owner.base() >= MAX_COMPANIES) || s.phase >= SpaceStructurePhase::End ||
			!IsSpaceStructureSiteValid(s, width, height) || s.content_version != MULTILAYER_SPACE_CONTENT_VERSION || s.capital_paid != d->capital ||
			r.work_ticks > d->work_ticks || r.age_ticks >= Ticks::CARGO_AGING_TICKS) return false;
		const bool complete = s.phase == SpaceStructurePhase::FrameComplete;
		if (complete != (r.work_ticks == d->work_ticks)) return false;
		const auto remaining = GetOrbitalConstructionRemainingEnergy(r, d->work_ticks);
		const bool worked = r.work_ticks != 0 || r.work_fraction != 0;
		// Exact work/remainder and same-file native receipts forbid unpaid fractions, future work or fractional completion.
		// New extension work must name its original allocation epoch; zero remains valid for portable Core tooling and migrated integer receipts.
		if (!remaining || r.last_work_tick > native_tick || (r.last_work_tick != 0 && !worked) ||
			(r.last_work_interval != 0 && r.last_work_tick == 0) || (r.work_fraction != 0 && r.last_work_tick == 0) ||
			(s.kind != SpaceBuildKind::Core && r.last_work_tick != 0 && r.last_work_interval == 0) ||
			(complete && *remaining != 0) || (s.kind == SpaceBuildKind::Core && (r.work_fraction != 0 || r.work_remainder != 0 || r.last_work_interval != 0))) return false;
		// Common material slots conserve required, delivered and consumed quantities across partial work and cancellation.
		for (uint i = 0; i < 2; ++i) {
			// Completion records exactly one whole-recipe consumption; partial active work retains the full material batch.
			if (r.quantities[i] > d->inputs[i].quantity || r.inputs[i].size() > r.quantities[i] || (r.quantities[i] == 0) != r.inputs[i].empty() ||
				r.consumed[i] != (complete ? d->inputs[i].quantity : 0) || (complete && r.quantities[i] != 0) ||
				(!complete && worked && s.phase == SpaceStructurePhase::Blueprint && r.quantities[i] != d->inputs[i].quantity)) return false;
			// Every referenced packet has one owner, retaining its valid routing destination.
			for (const auto &[packet, hop] : r.inputs[i]) if (packet >= CargoPacketID::End() || !packets.insert(packet).second || (hop != StationID::Invalid() && hop >= StationID::End())) return false;
		}
		if (r.module) {
			const auto &m = *r.module;
			// This producer activates Core only, after full consumption; pending activation cannot contain usable imported stock.
			if (!complete || s.kind != SpaceBuildKind::Core || m.activation_ticks > GetMultiLayerSpaceBudget().core_activation_ticks || m.age_ticks >= Ticks::CARGO_AGING_TICKS) return false;
			const auto &job = m.service; const bool active = job.remaining_ticks != 0;
			// Legacy uncommissioned modules have no consumer/history; active jobs require an actual commissioned source and berth.
			if (m.producer == UtilityProducerID{UINT32_MAX} && (m.consumer != UtilityConsumerID{UINT32_MAX} || m.assembly_consumer != UtilityConsumerID{UINT32_MAX} || active || m.services_completed != 0 || !m.power_enabled)) return false;
			if (active) {
				// The saved manifest serial and exclusive incarnation prevent another job or reused hull from claiming these parts.
				if (m.activation_ticks != 0 || m.consumer == UtilityConsumerID{UINT32_MAX} || job.remaining_ticks > GetMultiLayerSpaceBudget().service_ticks ||
					job.craft.vehicle == VehicleID::Invalid() || job.craft.generation == 0 || job.services_at_start == UINT64_MAX || m.services_completed == UINT64_MAX ||
					!service_craft.insert({job.craft.vehicle, job.craft.generation}).second) return false;
			} else {
				// An empty berth has one canonical representation; completed/cancelled jobs cannot hide a replayable work receipt.
				if (job.craft != SpaceCraftRef{} || job.paused || job.last_work_interval != 0 || job.services_at_start != 0) return false;
			}
			// Each reserved label contains exactly one actual part until the final atomic completion, never a quantity-only promise.
			for (uint i = 0; i < 2; ++i) {
				if (job.quantities[i] != (active ? 1 : 0) || job.inputs[i].size() != job.quantities[i]) return false;
				for (const auto &[packet, hop] : job.inputs[i]) if (packet >= CargoPacketID::End() || !packets.insert(packet).second || (hop != StationID::Invalid() && hop >= StationID::End())) return false;
			}
			// Each declared physical cargo stays bounded; packet identities cannot alias a job or another facility slot.
			for (CargoType cargo : EnumRange(NUM_CARGO)) {
				// Stock and packet-slot limits bound both gameplay capacity and saved transport before any facility can operate.
				if (m.quantities[cargo] > GetMultiLayerSpaceBudget().stock_cap || m.stock[cargo].size() > 64 || m.stock[cargo].size() > m.quantities[cargo] ||
					(m.quantities[cargo] == 0) != m.stock[cargo].empty() || (m.activation_ticks != 0 && m.quantities[cargo] != 0)) return false;
				for (const auto &[packet, hop] : m.stock[cargo]) if (packet >= CargoPacketID::End() || !packets.insert(packet).second || (hop != StationID::Invalid() && hop >= StationID::End())) return false;
			}
		}
	}
	// Pairwise site checks use only copied geometry; preview never consults or mutates the running world's pool.
	for (size_t i = 0; i < rows.size(); ++i) for (size_t j = i + 1; j < rows.size(); ++j) {
		/** @param r Copied site. @return Whether retained work or cargo still reserves its location. */
		auto reserves = [](const auto &r) { return r.state.phase != SpaceStructurePhase::Cancelled || r.quantities[0] != 0 || r.quantities[1] != 0; };
		if (reserves(rows[i]) && reserves(rows[j]) && SpaceStructureSitesOverlap(rows[i].state, rows[j].state)) return false;
	}
	return true;
}
/** Validate physical exclusivity after all other actual packet owners have installed.
 * @param rows Copied sites being validated or installed. @return Whether every packet exists exactly once with its saved quantity.
 */
static bool PhysicalInputs(std::span<const SpaceStructureRecord> rows)
{
	std::set<CargoPacketID> owned;
	/** @param list Actual external packet container. @return Whether its identities are exclusive. */
	auto add = [&](const auto &list) { for (const auto &[hop, packets] : *list.Packets()) { (void)hop; for (const auto *p : packets) if (!owned.insert(p->index).second) return false; } return true; };
	// Collect every other native physical owner before checking candidate orbital construction and completed stock.
	for (const auto *station : Station::Iterate()) {
		for (const auto &goods : station->goods) if (goods.HasData() && !add(goods.GetData().cargo)) return false;
		if (station->launch_hub) for (const auto &job : station->launch_hub->handling) if (!add(job->packets)) return false;
	}
	for (const auto *v : Vehicle::Iterate()) for (const auto *p : *v->cargo.Packets()) if (!owned.insert(p->index).second) return false;
	for (const auto *v : SpaceVehicle::Iterate()) if (v->manifest) for (const auto &slot : v->manifest->supplies) if (!add(slot)) return false;
	for (const auto *pod : SpacePod::Iterate()) if (pod->descent && !add(pod->descent->packets)) return false;
	for (const auto &r : rows) {
		// Existing company ownership must resolve; unowned retained cargo intentionally grants no operating rights.
		if (r.state.owner != OWNER_NONE && !Company::IsValidID(r.state.owner)) return false;
		for (uint i = 0; i < 2; ++i) {
			uint64_t count = 0;
			for (const auto &[id, hop] : r.inputs[i]) { (void)hop; const auto *p = CargoPacket::GetIfValid(id); if (p == nullptr || !owned.insert(id).second) return false; count += p->Count(); }
			if (count != r.quantities[i]) return false;
		}
		if (r.module) {
			// Bind the reserved RFMT/ELEC quantities to actual packet objects before any native job can be reconstructed.
			for (uint i = 0; i < 2; ++i) {
				uint64_t count = 0;
				for (const auto &[id, hop] : r.module->service.inputs[i]) { (void)hop; const auto *p = CargoPacket::GetIfValid(id); if (p == nullptr || !owned.insert(id).second) return false; count += p->Count(); }
				if (count != r.module->service.quantities[i]) return false;
			}
			// Stock quantities bind actual packet counts and cannot alias construction, vehicles, stations or descents.
			for (CargoType cargo : EnumRange(NUM_CARGO)) {
				uint64_t count = 0;
				for (const auto &[id, hop] : r.module->stock[cargo]) {
					(void)hop; const auto *p = CargoPacket::GetIfValid(id);
					if (p == nullptr || !owned.insert(id).second) return false;
					count += p->Count();
				}
				if (count != r.module->quantities[cargo]) return false;
				if (count != 0 && !IsValidCargoType(cargo)) return false;
			}
		}
	}
	return true;
}
/** @copydoc ValidateSpaceStructures */
bool ValidateSpaceStructures()
{
	const auto rows = CaptureSpaceStructures();
	return (rows.empty() || GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry) &&
		ValidateSpaceStructureRecords(rows, next_generation, Map::SizeX(), Map::SizeY(), TimerGameTick::counter) && PhysicalInputs(rows) && ValidateOrbitalServices();
}
/** @copydoc InstallLoadedSpaceStructures */
bool InstallLoadedSpaceStructures(std::span<const SpaceStructureRecord> rows, uint64_t next)
{
	// Installation cannot overwrite live owners or publish invalid/aliased packet inventories.
	if (SpaceStructure::GetNumItems() != 0 || !ValidateSpaceStructureRecords(rows, next, Map::SizeX(), Map::SizeY(), TimerGameTick::counter) || !PhysicalInputs(rows)) return false;
	// Reconstruct each sole native owner at its saved identity, preserving packet history without copying quantities.
	for (const auto &r : rows) {
		auto *site = SpaceStructure::CreateAtIndex(r.ref.structure); site->generation = r.ref.generation; site->state = r.state;
		static_cast<OrbitalConstructionState &>(site->job) = r; site->job.age_ticks = r.age_ticks; site->job.consumed = r.consumed;
		for (uint i = 0; i < 2; ++i) for (const auto &[id, hop] : r.inputs[i]) site->job.inputs[i].Append(CargoPacket::Get(id), hop);
		if (r.module) {
			site->module = std::make_unique<OrbitalModule>(); site->module->activation_ticks = r.module->activation_ticks; site->module->age_ticks = r.module->age_ticks;
			static_cast<OrbitalFacilityState &>(*site->module) = *r.module;
			static_cast<OrbitalServiceState &>(site->module->service) = r.module->service;
			// Reserved service containers preserve their sole owner and exact packet references across interrupted work.
			for (uint i = 0; i < 2; ++i) for (const auto &[id, hop] : r.module->service.inputs[i]) site->module->service.inputs[i].Append(CargoPacket::Get(id), hop);
			// Only actual referenced stock is installed; neither activation nor old-save migration supplies free material.
			for (CargoType cargo : EnumRange(NUM_CARGO)) for (const auto &[id, hop] : r.module->stock[cargo]) site->module->stock[cargo].Append(CargoPacket::Get(id), hop);
		}
	}
	const auto installed = CaptureSpaceStructures();
	// Container rebuilding must preserve saved packet ordering as well as aggregate amounts.
	for (const auto &r : rows) {
		/** @param s Installed copied site. @return Whether its full native identity matches. */
		const auto it = std::find_if(installed.begin(), installed.end(), [&](const auto &s) { return s.ref == r.ref; });
		if (it == installed.end() || it->inputs != r.inputs) return false;
		if (r.module && it->module->stock != r.module->stock) return false;
		if (r.module && it->module->service.inputs != r.module->service.inputs) return false;
	}
	next_generation = next; return true;
}
/** @copydoc ComputeSpaceStructureDigest */
uint64_t ComputeSpaceStructureDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	/** @param value Canonical integer whose big-endian bytes enter the native digest. */
	auto add = [&](uint64_t value) { for (int shift = 56; shift >= 0; shift -= 8) { digest ^= static_cast<uint8_t>(value >> shift); digest *= 1099511628211ULL; } };
	add(next_generation);
	// Hash authoritative state and packet provenance so interrupted execution cannot hide a duplicated delivery or renewed age.
	for (const auto &r : CaptureSpaceStructures()) {
		add(r.ref.structure.base()); add(r.ref.generation); add(r.state.owner.base()); add(to_underlying(r.state.kind)); add(to_underlying(r.state.phase)); add(r.state.x); add(r.state.y);
		add(r.state.content_version); add(r.state.capital_paid); add(r.work_ticks); add(r.age_ticks);
		add(r.work_fraction); add(r.work_remainder); add(r.last_work_tick); add(r.last_work_interval);
		for (uint i = 0; i < 2; ++i) {
			add(r.consumed[i]); add(r.quantities[i]); add(r.inputs[i].size());
			for (const auto &[id, hop] : r.inputs[i]) { const auto *p = CargoPacket::Get(id); add(id.base()); add(hop.base()); add(p->Count()); add(p->GetPeriodsInTransit()); add(static_cast<int64_t>(p->GetFeederShare())); add(p->GetSourceTile().base()); add(p->GetFirstStation().base()); add(to_underlying(p->GetSource().type)); add(p->GetSource().id); }
		}
		add(r.module.has_value());
		if (r.module) {
			add(r.module->activation_ticks); add(r.module->age_ticks);
			add(r.module->producer.base()); add(r.module->consumer.base()); add(r.module->assembly_consumer.base()); add(r.module->power_enabled); add(r.module->services_completed);
			const auto &job = r.module->service;
			add(job.craft.vehicle.base()); add(job.craft.generation); add(job.remaining_ticks); add(job.paused); add(job.last_work_interval); add(job.services_at_start);
			// Job packet provenance joins the digest independently of general inventory so reservation cannot disappear in a resave.
			for (uint i = 0; i < 2; ++i) {
				add(job.quantities[i]); add(job.inputs[i].size());
				for (const auto &[id, hop] : job.inputs[i]) {
					const auto *p = CargoPacket::Get(id); add(id.base()); add(hop.base()); add(p->Count()); add(p->GetPeriodsInTransit()); add(static_cast<int64_t>(p->GetFeederShare()));
					add(p->GetSourceTile().base()); add(p->GetFirstStation().base()); add(to_underlying(p->GetSource().type)); add(p->GetSource().id);
				}
			}
			// Separate cargo slots retain packet identity, routing and transport accounting through module custody.
			for (CargoType cargo : EnumRange(NUM_CARGO)) {
				add(r.module->quantities[cargo]); add(r.module->stock[cargo].size());
				for (const auto &[id, hop] : r.module->stock[cargo]) {
					const auto *p = CargoPacket::Get(id); add(id.base()); add(hop.base()); add(p->Count()); add(p->GetPeriodsInTransit()); add(static_cast<int64_t>(p->GetFeederShare()));
					add(p->GetSourceTile().base()); add(p->GetFirstStation().base()); add(to_underlying(p->GetSource().type)); add(p->GetSource().id);
				}
			}
		}
	}
	return digest;
}
