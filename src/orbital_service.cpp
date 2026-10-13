/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_service.cpp One paid Core berth, actual service packets and atomic shared-power settlement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1B/3.
 */
#include "stdafx.h"
#include "orbital_service.h"
#include "space_structure.h"
#include "space_structure_cmd.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "hub_cargo.h"
#include "company_base.h"
#include "company_func.h"
#include "command_func.h"
#include "core/backup_type.hpp"
#include "gfx_func.h"
#include "openttd.h"
#include "transport_tube.h"
#include "timer/timer_game_tick.h"
#include <set>
#include "safeguards.h"

/** @param craft Actual native hull. @return Its sole generation-qualified Core berth, or nullptr. */
static SpaceStructure *ServiceSite(const SpaceVehicle &craft)
{
	const auto ref = GetSpaceCraftReference(craft);
	// Scan contained jobs rather than maintaining a second assignment registry that could survive pool-ID reuse.
	for (auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		if (site->module->service.craft == ref) return site;
	}
	return nullptr;
}

/** @param site Native Core. @param craft Actual hull. @return Whether physical/ownership admission permits service work. */
static bool ReadyCraft(const SpaceStructure &site, const SpaceVehicle &craft)
{
	const auto model = GetSpaceVehicleModel(craft);
	return IsOrbitalModuleOperational(site) && craft.owner == site.state.owner &&
		(model == 15 || model == 19 || model == 20) && craft.construction.placement == SpacePlacement::Orbital &&
		craft.construction.content_version == MULTILAYER_SPACE_CONTENT_VERSION &&
		craft.position == GetSpaceStructureApproach(site.state) && craft.manifest != nullptr &&
		craft.manifest->service_ticks == 0 && craft.manifest->pending_returns == 0 &&
		!craft.vehstatus.Test(VehState::Stopped) && !craft.vehstatus.Test(VehState::Crashed);
}

/** @param site Native Core. @return Whether a real job still owns both parts and its exact unchanged manifest receipt. */
static bool ReadyJob(const SpaceStructure &site)
{
	if (!site.module) return false;
	const auto &m = *site.module; const auto &job = m.service;
	const auto *craft = ResolveSpaceCraft(job.craft);
	// Lost, foreign, paused or changed hulls retain their parts and berth, without spending power or renewing allowances.
	if (job.remaining_ticks == 0 || job.paused || craft == nullptr || !ReadyCraft(site, *craft) ||
		craft->manifest->services_completed != job.services_at_start || job.services_at_start == UINT64_MAX || m.services_completed == UINT64_MAX) return false;
	return job.inputs[0].AvailableCount() == 1 && job.inputs[1].AvailableCount() == 1;
}

/** @param stock Actual Core cargo slot. @return Number of retained physical packet references, not aggregate quantity. */
static size_t PacketCount(const StationCargoList &stock)
{
	size_t count = 0;
	// Saved transport bounds include every next-hop bucket; quantity alone cannot admit a fragmented destination.
	for (const auto &[hop, packets] : *stock.Packets()) { (void)hop; count += packets.size(); }
	return count;
}

/** @copydoc HasOrbitalServiceAssignment */
bool HasOrbitalServiceAssignment(const SpaceVehicle &craft) { return ServiceSite(craft) != nullptr; }

/** @copydoc HasOrbitalUtilitySources */
bool HasOrbitalUtilitySources()
{
	// Only actual commissioned sources keep the common interval driver running; declarations and legacy modules do not.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		const auto *source = GetUtilityPowerProducer(site->module->producer);
		if (source != nullptr && source->active && Company::IsValidID(site->state.owner)) return true;
	}
	return false;
}

/** @copydoc PublishOrbitalUtilitySources */
void PublishOrbitalUtilitySources(uint64_t interval, uint32_t ticks)
{
	// Publish every paid panel once in the existing common phase, including zero when activation or availability blocks it.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		const auto &m = *site->module; const auto *source = GetUtilityPowerProducer(m.producer);
		// Retired/unowned panels preserve their history but cannot become a reused company's free source.
		if (source == nullptr || !source->active || source->owner != site->state.owner || !Company::IsValidID(site->state.owner)) continue;
		AutoRestoreBackup actor(_current_company, source->owner);
		const auto rate = IsOrbitalModuleOperational(*site) && m.power_enabled ? GetMultiLayerSpaceBudget().core_panel_w : 0;
		PublishUtilityGeneration(m.producer, interval, rate, ticks);
	}
}

/** @copydoc AppendOrbitalUtilityRequests */
void AppendOrbitalUtilityRequests(std::vector<UtilityPowerRequest> &requests)
{
	// Gathering changes no timer, packet or allowance; the shared allocator owns every resulting reservation.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (!ReadyJob(*site)) continue;
		const auto *consumer = GetUtilityPowerConsumer(site->module->consumer);
		if (consumer != nullptr && consumer->active) requests.push_back({site->module->consumer, GetMultiLayerSpaceBudget().work_j_per_tick});
	}
}

/** @copydoc IsOrbitalServiceWorkTick */
bool IsOrbitalServiceWorkTick(const SpaceVehicle &craft)
{
	// Editor and paused callbacks never settle a retained simulation grant or accrue active-service upkeep.
	if (_game_mode != GameMode::Normal || _pause_mode.Any()) return false;
	if (GetTubePowerPreparedTick() != TimerGameTick::counter) return false;
	const auto *site = ServiceSite(craft);
	// Both upkeep and physical execution use this complete predicate; a paused or unpowered job earns no active tick.
	if (site == nullptr || !ReadyJob(*site) || !site->module->power_enabled || site->module->service.last_work_interval == GetUtilityPowerInterval()) return false;
	AutoRestoreBackup actor(_current_company, craft.owner);
	const auto grant = GetUtilityAllocation(site->module->consumer);
	return grant.has_value() && grant->granted_energy_j >= GetMultiLayerSpaceBudget().work_j_per_tick;
}

/** Whole physical participant; the utility owner commits its750J source debit with this exact service step. */
class OrbitalServiceStep final : public UtilityMutationParticipant {
	SpaceStructure &site;
public:
	/** @param site Actual native Core whose saved job owns the complete work transaction. */
	explicit OrbitalServiceStep(SpaceStructure &site) : site(site) {}
	/** @return Native assembly/service role; no generic810J response overrides Space's exact750J budget. */
	std::optional<UtilityConsumerEffect> EffectKind() const override { return UtilityConsumerEffect::AssemblyWork; }
	/** @param proposal Exact current utility transaction. @return Whether its quantity, epoch and complete physical job still agree. */
	bool Validate(const UtilityMutationProposal &proposal) const override
	{
		return ReadyJob(this->site) && proposal.grant.consumer == this->site.module->consumer &&
			proposal.quantity_j == GetMultiLayerSpaceBudget().work_j_per_tick && proposal.grant.interval > this->site.module->service.last_work_interval;
	}
	/** @param proposal Already revalidated whole transaction. @return Complete physical success; no ledger reentry or allocation occurs. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override
	{
		auto &m = *this->site.module; auto &job = m.service;
		job.last_work_interval = proposal.grant.interval;
		if (--job.remaining_ticks != 0) return true;
		// The final whole work tick consumes both exclusive parts once, then replaces rather than adds the bounded allowance.
		for (auto &slot : job.inputs) { const bool consumed = HubCargoTransfer::Consume(slot, 1); assert(consumed); (void)consumed; }
		auto &manifest = *ResolveSpaceCraft(job.craft)->manifest;
		++manifest.services_completed; manifest.returns_remaining = GetMultiLayerSpaceBudget().returns_per_service;
		++m.services_completed; static_cast<OrbitalServiceState &>(job) = {};
		return true;
	}
};

/** @copydoc TickOrbitalServices */
void TickOrbitalServices()
{
	// Native site order settles each single berth at most once against the already gathered common allocation.
	for (auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		auto *craft = ResolveSpaceCraft(site->module->service.craft);
		if (craft == nullptr) continue;
		if (!IsOrbitalServiceWorkTick(*craft)) continue;
		AutoRestoreBackup actor(_current_company, craft->owner);
		OrbitalServiceStep step(*site);
		const auto proposal = PrepareUtilityMutation(UtilityMutationKind::Consume,
			{site->module->consumer, GetUtilityPowerInterval()}, GetMultiLayerSpaceBudget().work_j_per_tick);
		CommitUtilityMutation(proposal, &step);
	}
}

/** @copydoc CmdCommissionOrbitalServices */
CommandCost CmdCommissionOrbitalServices(DoCommandFlags flags, SpaceStructureID id, uint64_t generation)
{
	auto *site = ResolveSpaceStructure({id, generation});
	// Migrated inventory-only cores commission explicitly; loading never creates a panel, stock or usable energy.
	if (site == nullptr || site->state.owner != _current_company || !IsOrbitalModuleOperational(*site) || site->module->producer != UtilityProducerID{UINT32_MAX}) return CMD_ERROR;
	const auto source = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::OrbitalPanel, flags.Test(DoCommandFlag::Execute));
	if (!source) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) site->module->producer = *source;
	return CommandCost{};
}

/** @copydoc CmdSetOrbitalPanelPaused */
CommandCost CmdSetOrbitalPanelPaused(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, bool paused)
{
	auto *site = ResolveSpaceStructure({id, generation});
	// An actual owned commissioned panel is required; inert declarations cannot manufacture power availability.
	if (site == nullptr || site->state.owner != _current_company || !IsOrbitalModuleOperational(*site) || site->module->producer == UtilityProducerID{UINT32_MAX}) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) site->module->power_enabled = !paused;
	return CommandCost{};
}

/** @copydoc CmdServiceOrbitalProbe */
CommandCost CmdServiceOrbitalProbe(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, VehicleID vehicle, uint64_t craft_generation, OrbitalServiceControl control)
{
	auto *site = ResolveSpaceStructure({id, generation}); const SpaceCraftRef ref{vehicle, craft_generation};
	// Commands address the exact owned operational Core; a lost occupant can still be cancelled to recover its parts.
	if (site == nullptr || site->state.owner != _current_company || !IsOrbitalModuleOperational(*site) || control >= OrbitalServiceControl::End) return CMD_ERROR;
	auto &m = *site->module; auto &job = m.service; const bool execute = flags.Test(DoCommandFlag::Execute);
	if (control != OrbitalServiceControl::Start) {
		// Control cannot retarget another job or renew a finished service through a stale command.
		if (job.remaining_ticks == 0 || job.craft != ref) return CMD_ERROR;
		if (control == OrbitalServiceControl::Cancel) {
			// Preflight both destination slots before returning anything; spent electricity is never refunded.
			for (uint i = 0; i < 2; ++i) {
				const auto cargo = GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1));
				if (!IsValidCargoType(cargo)) return CMD_ERROR;
				const auto &stock = m.stock[cargo];
				// Physical count and packet-record limits both constrain cancellation, preserving every part on refusal.
				if (stock.TotalCount() + 1 > GetOrbitalModuleCargoCapacity(*site, cargo) || PacketCount(stock) + PacketCount(job.inputs[i]) > 64 || !HubCargoTransfer::CanMove(job.inputs[i], 1)) return CMD_ERROR;
			}
			if (execute) {
				// Both whole single-unit inputs are already admitted; these moves require no split allocation.
				for (uint i = 0; i < 2; ++i) { const bool moved = HubCargoTransfer::Move(job.inputs[i], m.stock[GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1))], 1); assert(moved); (void)moved; }
				static_cast<OrbitalServiceState &>(job) = {};
			}
		} else if (execute) {
			job.paused = control == OrbitalServiceControl::Pause;
		}
		return CommandCost{};
	}
	auto *craft = ResolveSpaceCraft(ref); const auto *source = GetUtilityPowerProducer(m.producer);
	// One berth and one manifest receipt prevent onboard/Core overlap and duplicate allowance renewal.
	if (job.remaining_ticks != 0 || craft == nullptr || !ReadyCraft(*site, *craft) || HasOrbitalServiceAssignment(*craft) ||
		source == nullptr || !source->active || source->owner != _current_company || craft->manifest->services_completed == UINT64_MAX || m.services_completed == UINT64_MAX) return CMD_ERROR;
	// Reserve both actual labels before any split/registration; missing either material leaves the complete stock untouched.
	for (uint i = 0; i < 2; ++i) {
		const auto cargo = GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1));
		if (!IsValidCargoType(cargo)) return CMD_ERROR;
		if (!HubCargoTransfer::CanMove(m.stock[cargo], 1)) return CMD_ERROR;
	}
	if (!CargoPacket::CanAllocateItem(2)) return CMD_ERROR;
	const std::array sources{m.producer};
	if (m.consumer == UtilityConsumerID{UINT32_MAX}) {
		const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork,
			GetMultiLayerSpaceBudget().work_j_per_tick, sources, execute);
		if (!consumer) return CMD_ERROR;
		if (execute) m.consumer = *consumer;
	}
	if (execute) {
		// No intervening mutation can consume the preflighted packet capacity; both parts become exclusive to this job.
		for (uint i = 0; i < 2; ++i) { const bool moved = HubCargoTransfer::Move(m.stock[GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1))], job.inputs[i], 1); assert(moved); (void)moved; }
		job.craft = ref; job.remaining_ticks = GetMultiLayerSpaceBudget().service_ticks; job.services_at_start = craft->manifest->services_completed;
		MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc CmdMoveOrbitalSupply */
CommandCost CmdMoveOrbitalSupply(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, VehicleID vehicle, uint64_t craft_generation, uint8_t supply, uint16_t quantity, bool load)
{
	auto *site = ResolveSpaceStructure({id, generation}); auto *craft = ResolveSpaceCraft({vehicle, craft_generation});
	// Only real service compartments at an idle owned precise berth may exchange RFMT/ELEC; fuel remains a separate function.
	if (site == nullptr || craft == nullptr || site->state.owner != _current_company || !ReadyCraft(*site, *craft) ||
		site->module->service.remaining_ticks != 0 || HasOrbitalServiceAssignment(*craft) || supply < 1 || supply > 2 || quantity == 0) return CMD_ERROR;
	const auto cargo = GetSpaceSupplyCargo(static_cast<SpaceSupply>(supply));
	if (!IsValidCargoType(cargo)) return CMD_ERROR;
	auto &stock = site->module->stock[cargo]; auto &hold = craft->manifest->supplies[supply];
	auto &from = load ? stock : hold; auto &to = load ? hold : stock;
	// The common paid service buffers use the same frozen RFMT64/ELEC32 defaults at Earth and in the first Core;
	// general1024-unit freight capacity must not enlarge a declared service compartment through this exchange command.
	const auto &budget = GetMultiLayerSpaceBudget();
	const auto station_capacity = supply == 1 ? budget.launch_metal_cap : budget.launch_electronics_cap;
	const auto capacity = load ? GetSpaceSupplyCapacity(GetSpaceVehicleModel(*craft), static_cast<SpaceSupply>(supply)) : station_capacity;
	// Destination quantity and fragmented packet bounds are admitted together before any unpaid physical transfer.
	if (to.TotalCount() + quantity > capacity || (!load && PacketCount(to) + PacketCount(from) > 64) || !HubCargoTransfer::CanMove(from, quantity)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) { const bool moved = HubCargoTransfer::Move(from, to, quantity); assert(moved); (void)moved; craft->MarkDirty(); }
	return CommandCost{};
}

/** @copydoc ValidateOrbitalServiceUtilityRecords */
bool ValidateOrbitalServiceUtilityRecords(std::span<const SpaceStructureRecord> rows, const UtilityPowerState &power)
{
	std::set<UtilityProducerID> producers; std::set<UtilityConsumerID> consumers;
	// Both preview and actual-load validation use the same copied typed bindings, never feature-local energy accounts.
	for (const auto &site : rows) {
		if (site.last_work_interval > power.interval) return false;
		if (!site.module) continue;
		const auto &m = *site.module; const auto &job = m.service;
		if (m.producer != UtilityProducerID{UINT32_MAX}) {
			/** @param p Copied source. @return Whether this is the original issued panel identity. */
			const auto p = std::find_if(power.producers.begin(), power.producers.end(), [&](const auto &p) { return p.meter.producer_id == m.producer; });
			// Original typed panel identities cannot alias another Core or another source family/layer.
			if (p == power.producers.end() || !producers.insert(m.producer).second || p->kind != UtilityGenerationKind::OrbitalPanel || p->meter.layer != MapLayer::Space ||
				p->meter.domain != UtilityEnergyDomain::Electricity || (site.state.owner != OWNER_NONE && p->owner != site.state.owner)) return false;
		}
		// Service and construction are separate bounded physical slots sharing the same original paid panel.
		for (const auto id : {m.consumer, m.assembly_consumer}) {
			if (id == UtilityConsumerID{UINT32_MAX}) continue;
			/** @param c Copied consumer. @return Whether this is the original issued bounded work identity. */
			const auto c = std::find_if(power.consumers.begin(), power.consumers.end(), [&](const auto &c) { return c.meter.consumer_id == id; });
			// Each live work slot is dedicated to its panel; company removal clears retired eligibility while retaining original paid history.
			if (c == power.consumers.end() || !consumers.insert(id).second || c->effect != UtilityConsumerEffect::AssemblyWork || c->meter.layer != MapLayer::Space ||
				c->meter.domain != UtilityEnergyDomain::Electricity || c->capacity_j != GetMultiLayerSpaceBudget().work_j_per_tick ||
				(c->active ? c->linked_producers != std::vector{m.producer} : !c->linked_producers.empty()) ||
				(site.state.owner != OWNER_NONE && c->owner != site.state.owner)) return false;
		}
		if (job.last_work_interval > power.interval) return false;
	}
	return true;
}

/** @copydoc ValidateOrbitalServices */
bool ValidateOrbitalServices()
{
	if (!ValidateOrbitalServiceUtilityRecords(CaptureSpaceStructures(), CaptureUtilityPowerState())) return false;
	// Resolve hulls only for actual-world acceptance; copied preview performs the same checks against its own native catalogue.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		const auto &job = site->module->service;
		if (job.remaining_ticks == 0) continue;
		if (job.craft.generation >= GetNextSpaceVehicleGeneration()) return false;
		const auto *craft = ResolveSpaceCraft(job.craft);
		// Removed hulls retain waiting parts; a still-live incarnation must retain its same owner and native service serial.
		if (craft != nullptr && (craft->owner != site->state.owner || !craft->manifest || craft->manifest->service_ticks != 0 || craft->manifest->services_completed != job.services_at_start)) return false;
		if (craft != nullptr) {
			const auto model = GetSpaceVehicleModel(*craft);
			// A genuine paid deployed probe is the only legal berth occupant; rockets and historical preview hulls cannot acquire service rights.
			if ((model != 15 && model != 19 && model != 20) || craft->construction.placement != SpacePlacement::Orbital || craft->construction.content_version != MULTILAYER_SPACE_CONTENT_VERSION) return false;
		}
	}
	return true;
}
