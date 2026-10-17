/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_service.cpp Contained Core/Dock berths, actual service packets and atomic shared-power settlement.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1B/3/4C/6B.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC; original contained timers consume complete per-role bills through the same750J owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C; the same native connection commands delegate to the distinct processing owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG; original countdown queried for presentation only.
 */
#include "stdafx.h"
#include "orbital_service.h"
#include "orbital_manufacturing.h"
#include "orbital_recipe.h"
#include "space_structure.h"
#include "space_approach.h"
#include "space_structure_cmd.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_mining_tuning.h"
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

/** Exact contained assignment; this transient view owns no craft, berth, part or utility account. */
struct OrbitalServiceAssignment {
	SpaceStructure *site;
	OrbitalServiceJob *job;
	uint8_t berth;
};

/** @copydoc GetOrbitalServiceJobTuning */
std::optional<SpaceServiceTuning> GetOrbitalServiceJobTuning(const OrbitalServiceState &job)
{
	// Genuine older receipts never contained a model. Preserve their original bounded bill after load/loss rather than reconstructing a current role from another hull or assuming heavy rights.
	if (job.model == UINT16_MAX) return SpaceServiceTuning{GetMultiLayerSpaceBudget().service_ticks, {1, 1}, GetMultiLayerSpaceBudget().returns_per_service};
	return GetSpaceServiceTuning(job.model);
}

/** @param kind Actual paid module role. @return Bounded electrical work slots; an Assembly work slot grants no probe service berth. */
static uint8_t OrbitalElectricalWorkCapacity(SpaceBuildKind kind)
{
	return kind == SpaceBuildKind::Assembly ? 1 : GetOrbitalModuleServiceCapacity(kind);
}

/** @copydoc GetOrbitalServicePowerSite */
const SpaceStructure *GetOrbitalServicePowerSite(const SpaceStructure &site)
{
	if (!IsOrbitalModuleOperational(site)) return nullptr;
	if (site.state.kind == SpaceBuildKind::Core) return &site;
	// Dock service and Assembly manufacture share explicit source geometry; other modules cannot borrow an arbitrary nearby panel.
	if (site.state.kind != SpaceBuildKind::Dock && site.state.kind != SpaceBuildKind::Assembly) return nullptr;
	auto *source = ResolveSpaceStructure(site.module->utility_source);
	// Only the exact original adjacent same-company panel supplies work; a reused ID or unrelated nearby producer cannot supply free work.
	if (source == nullptr || source->state.owner != site.state.owner || !IsOrbitalModuleOperational(*source) ||
		(source->state.kind != SpaceBuildKind::Core && source->state.kind != SpaceBuildKind::Power) ||
		!AreOrbitalModuleFootprintsAdjacent(site.state, source->state)) return nullptr;
	return source;
}

/** @param craft Actual native hull. @return Its sole generation-qualified contained service berth, if any. */
static std::optional<OrbitalServiceAssignment> ServiceSite(const SpaceVehicle &craft)
{
	const auto ref = GetSpaceCraftReference(craft);
	// Scan contained jobs rather than maintaining a second assignment registry that could survive pool-ID reuse.
	for (auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		const auto jobs = site->module->GetServiceJobs();
		// Assignment follows the actual retained job even after a hull is lost or its physical source becomes unavailable.
		for (uint8_t berth = 0; berth < jobs.size(); ++berth) if (jobs[berth]->craft == ref) return OrbitalServiceAssignment{site, jobs[berth], berth};
	}
	return std::nullopt;
}

/** @param site Native Core. @param craft Actual hull. @return Whether physical/ownership admission permits service work. */
static bool ReadyCraft(const SpaceStructure &site, const SpaceVehicle &craft)
{
	const auto model = GetSpaceVehicleModel(craft);
	// The original Core berth preserves historical paid service. Heavy Core and every Dock job require the exact shared admitted physical slot, so a coincident position cannot create new service rights.
	const bool at_berth = site.state.kind == SpaceBuildKind::Core && model != SPACE_HEAVY_MINER_MODEL ? craft.position == GetSpaceStructureApproach(site.state) : GetSpaceStructureBerth(craft, site).has_value();
	return GetOrbitalModuleServiceCapacity(site.state.kind) != 0 && IsOrbitalModuleOperational(site) && craft.owner == site.state.owner &&
		GetSpaceServiceTuning(model).has_value() && craft.construction.placement == SpacePlacement::Orbital &&
		IsSupportedSpaceVehicleContentVersion(craft.construction.content_version, model) &&
		at_berth && craft.manifest != nullptr &&
		craft.manifest->service_ticks == 0 && craft.manifest->pending_returns == 0 &&
		(model != SPACE_HEAVY_MINER_MODEL || craft.mission.phase == SpaceMissionPhase::Holding) &&
		!craft.vehstatus.Test(VehState::Stopped) && !craft.vehstatus.Test(VehState::Crashed);
}

/** @param site Native service module. @param job Its actual contained job. @return Whether physical parts, berth and the manifest's original serial permit work. */
static bool ReadyJob(const SpaceStructure &site, const OrbitalServiceJob &job)
{
	if (!site.module) return false;
	const auto &m = *site.module;
	const auto *craft = ResolveSpaceCraft(job.craft);
	// Lost, foreign, paused or changed hulls retain their parts and berth, without spending power or renewing allowances.
	if (job.remaining_ticks == 0 || job.paused || craft == nullptr || !ReadyCraft(site, *craft) ||
		craft->manifest->services_completed != job.services_at_start || job.services_at_start == UINT64_MAX || m.services_completed == UINT64_MAX) return false;
	const auto model = GetSpaceVehicleModel(*craft); const auto tuning = GetOrbitalServiceJobTuning(job);
	// An immutable bill follows only its original role. Unknown old receipts remain legacy-only; heavy work requires its own known role and exhausted allowance throughout execution.
	if (!tuning || job.remaining_ticks > tuning->ticks || (job.model == UINT16_MAX ? model == SPACE_HEAVY_MINER_MODEL : model != job.model) ||
		(model == SPACE_HEAVY_MINER_MODEL && craft->manifest->returns_remaining != 0)) return false;
	const auto *source = GetOrbitalServicePowerSite(site);
	if (source == nullptr) return false;
	if (site.state.kind == SpaceBuildKind::Dock) {
		const auto berth = GetSpaceStructureBerth(*craft, site); const auto jobs = site.module->GetServiceJobs();
		// A hull admitted to one Dock berth cannot spend parts reserved by the other physical job.
		if (!berth || jobs[*berth] != &job) return false;
	}
	return job.inputs[0].AvailableCount() == tuning->inputs[0] && job.inputs[1].AvailableCount() == tuning->inputs[1];
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
bool HasOrbitalServiceAssignment(const SpaceVehicle &craft) { return ServiceSite(craft).has_value(); }

/** @copydoc GetOrbitalServiceJobState */
const OrbitalServiceState *GetOrbitalServiceJobState(const SpaceVehicle &craft)
{
	const auto assignment = ServiceSite(craft);
	return assignment ? assignment->job : nullptr;
}

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
		const auto rate = GetOrbitalModulePanelRate(*site);
		PublishUtilityGeneration(m.producer, interval, rate, ticks);
	}
}

/** @copydoc AppendOrbitalUtilityRequests */
void AppendOrbitalUtilityRequests(std::vector<UtilityPowerRequest> &requests)
{
	// Gathering changes no timer, packet or allowance; the shared allocator owns every resulting reservation.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		uint64_t demand = 0;
		// The one bounded service consumer requests only its actual eligible jobs; two Dock berths cannot enlarge their declared per-job work budget.
		for (const auto *job : site->module->GetServiceJobs()) if (ReadyJob(*site, *job)) demand += GetMultiLayerSpaceBudget().work_j_per_tick;
		if (demand == 0) continue;
		const auto *consumer = GetUtilityPowerConsumer(site->module->consumer);
		if (consumer != nullptr && consumer->active) requests.push_back({site->module->consumer, demand});
	}
}

/** @copydoc IsOrbitalServiceWorkTick */
bool IsOrbitalServiceWorkTick(const SpaceVehicle &craft)
{
	// Editor and paused callbacks never settle a retained simulation grant or accrue active-service upkeep.
	if (_game_mode != GameMode::Normal || _pause_mode.Any()) return false;
	if (GetTubePowerPreparedTick() != TimerGameTick::counter) return false;
	const auto assignment = ServiceSite(craft);
	// Both upkeep and physical execution use this complete predicate; a paused or unpowered job earns no active tick.
	if (!assignment || !ReadyJob(*assignment->site, *assignment->job) || assignment->job->last_work_interval == GetUtilityPowerInterval()) return false;
	const auto *source = GetOrbitalServicePowerSite(*assignment->site);
	// Physical source pause/loss blocks stale grants immediately; demand remains visible to the shared ledger without earning work.
	if (source == nullptr || GetOrbitalModulePanelRate(*source) == 0) return false;
	AutoRestoreBackup actor(_current_company, craft.owner);
	const auto grant = GetUtilityAllocation(assignment->site->module->consumer);
	uint64_t required = GetMultiLayerSpaceBudget().work_j_per_tick;
	const auto jobs = assignment->site->module->GetServiceJobs();
	// Earlier eligible unsettled berths have first claim on the same remaining ledger grant, keeping upkeep identical before and after their commits.
	for (uint8_t berth = 0; berth < assignment->berth; ++berth) {
		if (ReadyJob(*assignment->site, *jobs[berth]) && jobs[berth]->last_work_interval != GetUtilityPowerInterval()) required += GetMultiLayerSpaceBudget().work_j_per_tick;
	}
	return grant.has_value() && grant->granted_energy_j >= required;
}

/** Whole physical participant; the utility owner commits its750J source debit with this exact service step. */
class OrbitalServiceStep final : public UtilityMutationParticipant {
	SpaceStructure &site;
	OrbitalServiceJob &job;
public:
	/** @param site Actual service module. @param job Its exact contained berth whose saved receipt owns this complete work transaction. */
	OrbitalServiceStep(SpaceStructure &site, OrbitalServiceJob &job) : site(site), job(job) {}
	/** @return Native assembly/service role; no generic810J response overrides Space's exact750J budget. */
	std::optional<UtilityConsumerEffect> EffectKind() const override { return UtilityConsumerEffect::AssemblyWork; }
	/** @param proposal Exact current utility transaction. @return Whether its quantity, epoch and complete physical job still agree. */
	bool Validate(const UtilityMutationProposal &proposal) const override
	{
		return ReadyJob(this->site, this->job) && proposal.grant.consumer == this->site.module->consumer &&
			proposal.quantity_j == GetMultiLayerSpaceBudget().work_j_per_tick && proposal.grant.interval > this->job.last_work_interval;
	}
	/** @param proposal Already revalidated whole transaction. @return Complete physical success; no ledger reentry or allocation occurs. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override
	{
		auto &m = *this->site.module; auto &job = this->job;
		job.last_work_interval = proposal.grant.interval;
		if (--job.remaining_ticks != 0) return true;
		const auto tuning = GetOrbitalServiceJobTuning(job); assert(tuning);
		// The final whole work tick consumes both complete immutable input bills once, then replaces rather than adds the bounded allowance. Earlier opportunities retain every actual part.
		for (uint i = 0; i < 2; ++i) { const bool consumed = HubCargoTransfer::Consume(job.inputs[i], tuning->inputs[i]); assert(consumed); (void)consumed; }
		auto &manifest = *ResolveSpaceCraft(job.craft)->manifest;
		++manifest.services_completed; manifest.returns_remaining = tuning->returns;
		++m.services_completed; static_cast<OrbitalServiceState &>(job) = {};
		return true;
	}
};

/** @copydoc TickOrbitalServices */
void TickOrbitalServices()
{
	// Native site then contained-berth order settles each actual job once against the same already gathered common allocation.
	for (auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		for (auto *job : site->module->GetServiceJobs()) {
			auto *craft = ResolveSpaceCraft(job->craft);
			if (craft == nullptr) continue;
			if (!IsOrbitalServiceWorkTick(*craft)) continue;
			AutoRestoreBackup actor(_current_company, craft->owner);
			OrbitalServiceStep step(*site, *job);
			const auto proposal = PrepareUtilityMutation(UtilityMutationKind::Consume,
				{site->module->consumer, GetUtilityPowerInterval()}, GetMultiLayerSpaceBudget().work_j_per_tick);
			CommitUtilityMutation(proposal, &step);
		}
	}
}

/** @copydoc CmdCommissionOrbitalServices */
CommandCost CmdCommissionOrbitalServices(DoCommandFlags flags, SpaceStructureID id, uint64_t generation)
{
	auto *site = ResolveSpaceStructure({id, generation});
	// Migrated inventory-only cores commission explicitly; loading never creates a panel, stock or usable energy.
	if (site == nullptr || site->state.kind != SpaceBuildKind::Core || site->state.owner != _current_company || !IsOrbitalModuleOperational(*site) || site->module->producer != UtilityProducerID{UINT32_MAX}) return CMD_ERROR;
	const auto source = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::OrbitalPanel, flags.Test(DoCommandFlag::Execute));
	if (!source) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) site->module->producer = *source;
	return CommandCost{};
}

/** @copydoc CmdConnectOrbitalServices */
CommandCost CmdConnectOrbitalServices(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, SpaceStructureID source_id, uint64_t source_generation)
{
	auto *site = ResolveSpaceStructure({id, generation}); const auto *source = ResolveSpaceStructure({source_id, source_generation});
	// The same typed original source command reaches a processor's distinct metadata/consumer owner; it never grants that module a Dock berth or Assembly manufacturing account.
	if (site != nullptr && IsOrbitalRecipeKind(site->state.kind)) {
		if (source == nullptr) return CMD_ERROR;
		return ConnectOrbitalRecipePower(flags, *site, *source);
	}
	// A connection follows paid Dock/Assembly activation and an actual owned physical edge; it cannot manufacture a panel or inherit a reused source identity.
	if (site == nullptr || source == nullptr || !Company::IsValidID(_current_company) || site->state.owner != _current_company || source->state.owner != _current_company ||
		(site->state.kind != SpaceBuildKind::Dock && site->state.kind != SpaceBuildKind::Assembly) || !IsOrbitalModuleOperational(*site) || !IsOrbitalModuleOperational(*source) ||
		(source->state.kind != SpaceBuildKind::Core && source->state.kind != SpaceBuildKind::Power) || !AreOrbitalModuleFootprintsAdjacent(site->state, source->state)) return CMD_ERROR;
	auto &module = *site->module; const auto *producer = GetUtilityPowerProducer(source->module->producer);
	// Register the one original bounded work consumer once; source toggles and subsequent service jobs do not reissue IDs or grant energy.
	if (module.consumer != UtilityConsumerID{UINT32_MAX} || module.utility_source != SpaceStructureRef{} || producer == nullptr || !producer->active ||
		producer->owner != _current_company || producer->meter.layer != MapLayer::Space || producer->kind != UtilityGenerationKind::OrbitalPanel) return CMD_ERROR;
	const std::array sources{source->module->producer}; const bool execute = flags.Test(DoCommandFlag::Execute);
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork,
		uint64_t{OrbitalElectricalWorkCapacity(site->state.kind)} * GetMultiLayerSpaceBudget().work_j_per_tick, sources, execute);
	if (!consumer) return CMD_ERROR;
	if (execute) { module.consumer = *consumer; module.utility_source = GetSpaceStructureReference(*source); MarkWholeScreenDirty(); }
	return CommandCost{};
}

/** @copydoc CmdDisconnectOrbitalServices */
CommandCost CmdDisconnectOrbitalServices(DoCommandFlags flags, SpaceStructureID id, uint64_t generation)
{
	auto *site = ResolveSpaceStructure({id, generation});
	// Original processing cleanup preserves its private paid-work/history boundary even when source/Core availability has been lost.
	if (site != nullptr && IsOrbitalRecipeKind(site->state.kind)) return DisconnectOrbitalRecipePower(flags, *site);
	// Paid work facilities retain history through outages; disconnection never requires their original source to still operate.
	if (site == nullptr || !Company::IsValidID(_current_company) || site->state.owner != _current_company ||
		(site->state.kind != SpaceBuildKind::Dock && site->state.kind != SpaceBuildKind::Assembly) ||
		!site->module || site->module->activation_ticks != 0 || site->module->utility_source == SpaceStructureRef{} || site->module->consumer == UtilityConsumerID{UINT32_MAX}) return CMD_ERROR;
	auto &module = *site->module;
	// A queued or reserved vehicle order owns this exact work connection until explicit cancellation; disconnect cannot strand its paid reservation.
	if (module.manufacture) return CMD_ERROR;
	// Every held job is an accountable physical owner; the player must return/cancel its actual parts before retiring its shared work connection.
	for (const auto *job : module.GetServiceJobs()) if (job->remaining_ticks != 0 || job->inputs[0].TotalCount() != 0 || job->inputs[1].TotalCount() != 0) return CMD_ERROR;
	const std::array consumers{module.consumer}; const bool execute = flags.Test(DoCommandFlag::Execute);
	if (RetireOrbitalUtilityAccounts({}, consumers, execute) != UtilityPowerError::None) return CMD_ERROR;
	if (execute) { module.consumer = UtilityConsumerID{UINT32_MAX}; module.utility_source = {}; MarkWholeScreenDirty(); }
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
	if (flags.Test(DoCommandFlag::QueryCost)) flags.Reset(DoCommandFlag::Execute);
	auto *site = ResolveSpaceStructure({id, generation}); const SpaceCraftRef ref{vehicle, craft_generation};
	// Retained paid berths allow explicit cancellation during a dependency outage, so lost power/Core never strands actual unconsumed parts.
	if (site == nullptr || !Company::IsValidID(_current_company) || site->state.owner != _current_company || site->module == nullptr ||
		site->state.phase != SpaceStructurePhase::FrameComplete || site->module->activation_ticks != 0 || GetOrbitalModuleServiceCapacity(site->state.kind) == 0 || control >= OrbitalServiceControl::End) return CMD_ERROR;
	auto &m = *site->module; const bool execute = flags.Test(DoCommandFlag::Execute); OrbitalServiceJob *selected = nullptr;
	const auto jobs = m.GetServiceJobs();
	// Control follows the job's original exact craft reference, even after that hull disappears or another craft is selected in the UI.
	for (auto *candidate : jobs) if (candidate->craft == ref && candidate->remaining_ticks != 0) selected = candidate;
	if (control != OrbitalServiceControl::Start) {
		// Control cannot retarget another job or renew a finished service through a stale command.
		if (selected == nullptr) return CMD_ERROR;
		auto &job = *selected;
		const auto tuning = GetOrbitalServiceJobTuning(job); if (!tuning) return CMD_ERROR;
		if (control == OrbitalServiceControl::Cancel) {
			// Preflight both destination slots before returning anything; spent electricity is never refunded.
			for (uint i = 0; i < 2; ++i) {
				const auto cargo = GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1));
				if (!IsValidCargoType(cargo)) return CMD_ERROR;
				const auto &stock = m.stock[cargo];
				// Physical count and packet-record limits both constrain cancellation, preserving every part on refusal.
				if (stock.TotalCount() + tuning->inputs[i] > GetOrbitalModuleStockLimit(site->state.kind) || PacketCount(stock) + PacketCount(job.inputs[i]) > 64 || !HubCargoTransfer::CanMove(job.inputs[i], tuning->inputs[i])) return CMD_ERROR;
			}
			if (execute) {
				// Both complete immutable bills are already admitted; cancellation returns only those actual reserved packets and refunds no earned electrical work.
				for (uint i = 0; i < 2; ++i) { const bool moved = HubCargoTransfer::Move(job.inputs[i], m.stock[GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1))], tuning->inputs[i]); assert(moved); (void)moved; }
				static_cast<OrbitalServiceState &>(job) = {};
			}
		} else if (execute) {
			job.paused = control == OrbitalServiceControl::Pause;
		}
		return CommandCost{};
	}
	auto *craft = ResolveSpaceCraft(ref);
	if (craft == nullptr) return CMD_ERROR;
	const auto model = GetSpaceVehicleModel(*craft); const auto tuning = GetSpaceServiceTuning(model); if (!tuning) return CMD_ERROR;
	// The new heavy producer renews only exhausted allowance. Historical15/19/20 jobs retain their original admission rather than being reinterpreted under a later mining policy.
	if (model == SPACE_HEAVY_MINER_MODEL && (!craft->manifest || craft->manifest->returns_remaining != 0)) return CMD_ERROR;
	const auto berth = site->state.kind == SpaceBuildKind::Core ? std::optional<uint8_t>{0} : GetSpaceStructureBerth(*craft, *site);
	if (!berth) return CMD_ERROR;
	auto &job = *jobs[*berth]; const auto *source_site = GetOrbitalServicePowerSite(*site);
	const auto *source = source_site == nullptr ? nullptr : GetUtilityPowerProducer(source_site->module->producer);
	// One berth and one manifest receipt prevent onboard/Core overlap and duplicate allowance renewal.
	if (job.remaining_ticks != 0 || craft == nullptr || !ReadyCraft(*site, *craft) || HasOrbitalServiceAssignment(*craft) ||
		source == nullptr || !source->active || source->owner != _current_company || (site->state.kind == SpaceBuildKind::Dock && m.consumer == UtilityConsumerID{UINT32_MAX}) ||
		craft->manifest->services_completed == UINT64_MAX || m.services_completed == UINT64_MAX) return CMD_ERROR;
	// Reserve both actual labels before any split/registration; missing either material leaves the complete stock untouched.
	for (uint i = 0; i < 2; ++i) {
		const auto cargo = GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1));
		if (!IsValidCargoType(cargo)) return CMD_ERROR;
		if (!HubCargoTransfer::CanMove(m.stock[cargo], tuning->inputs[i])) return CMD_ERROR;
	}
	if (!CargoPacket::CanAllocateItem(2)) return CMD_ERROR;
	const std::array sources{source_site->module->producer};
	if (m.consumer == UtilityConsumerID{UINT32_MAX}) {
		const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork,
			GetMultiLayerSpaceBudget().work_j_per_tick, sources, execute);
		if (!consumer) return CMD_ERROR;
		if (execute) m.consumer = *consumer;
	}
	if (execute) {
		// No intervening mutation can consume the preflighted packet capacity; both parts become exclusive to this job.
		for (uint i = 0; i < 2; ++i) { const bool moved = HubCargoTransfer::Move(m.stock[GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1))], job.inputs[i], tuning->inputs[i]); assert(moved); (void)moved; }
		job.craft = ref; job.model = model; job.remaining_ticks = tuning->ticks; job.services_at_start = craft->manifest->services_completed;
		MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc CmdMoveOrbitalSupply */
CommandCost CmdMoveOrbitalSupply(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, VehicleID vehicle, uint64_t craft_generation, uint8_t supply, uint16_t quantity, bool load)
{
	if (flags.Test(DoCommandFlag::QueryCost)) flags.Reset(DoCommandFlag::Execute);
	auto *site = ResolveSpaceStructure({id, generation}); auto *craft = ResolveSpaceCraft({vehicle, craft_generation});
	// Only real service compartments at an idle owned precise berth may exchange RFMT/ELEC; fuel remains a separate function.
	if (site == nullptr || craft == nullptr || site->state.owner != _current_company || !ReadyCraft(*site, *craft) ||
		HasOrbitalServiceAssignment(*craft) || supply < 1 || supply > 2 || quantity == 0) return CMD_ERROR;
	const auto berth = site->state.kind == SpaceBuildKind::Core ? std::optional<uint8_t>{0} : GetSpaceStructureBerth(*craft, *site);
	// Parts exchange uses only this craft's idle admitted berth; work in the Dock's other berth does not consume this handling capacity.
	if (!berth || site->module->GetServiceJobs()[*berth]->remaining_ticks != 0) return CMD_ERROR;
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
	if (!ValidateOrbitalManufacturingRecords(rows, power.interval)) return false;
	std::set<UtilityProducerID> producers; std::set<UtilityConsumerID> consumers;
	// Both preview and actual-load validation use the same copied typed bindings, never feature-local energy accounts.
	for (const auto &site : rows) {
		if (site.last_work_interval > power.interval) return false;
		if (!site.module) continue;
		const auto &m = *site.module; UtilityProducerID service_producer = m.producer;
		const bool external_work = site.state.kind == SpaceBuildKind::Dock || site.state.kind == SpaceBuildKind::Assembly;
		// Both paid external work roles retain an original panel connection; neither copied own-panel identity nor unconnected consumer is admissible.
		if (external_work && (m.producer != UtilityProducerID{UINT32_MAX} ||
			((m.consumer == UtilityConsumerID{UINT32_MAX}) != (m.utility_source == SpaceStructureRef{})))) return false;
		// A retained connection resolves within the same snapshot, preventing an active-world panel or a reused native ID from legitimizing it.
		if (external_work && m.utility_source != SpaceStructureRef{}) {
			/** @param candidate Same-file native site. @return Whether it retains the exact original physical source incarnation. */
			const auto source = std::ranges::find_if(rows, [&](const auto &candidate) { return candidate.ref == m.utility_source; });
			// Copied preview binds only its own original paid adjacent panel; no active-world source or copied numeric producer ID may substitute.
			if (source == rows.end() || !source->module || source->state.phase != SpaceStructurePhase::FrameComplete || source->module->activation_ticks != 0 ||
				(source->state.kind != SpaceBuildKind::Core && source->state.kind != SpaceBuildKind::Power) || source->state.owner != site.state.owner ||
				!AreOrbitalModuleFootprintsAdjacent(site.state, source->state) || source->module->producer == UtilityProducerID{UINT32_MAX}) return false;
			service_producer = source->module->producer;
		}
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
			const bool external_consumer = external_work && id == m.consumer;
			const uint64_t capacity = uint64_t{GetMultiLayerSpaceBudget().work_j_per_tick} * (external_consumer ? OrbitalElectricalWorkCapacity(site.state.kind) : 1);
			const auto expected_source = external_consumer ? service_producer : m.producer;
			if (c == power.consumers.end() || !consumers.insert(id).second || c->effect != UtilityConsumerEffect::AssemblyWork || c->meter.layer != MapLayer::Space ||
				c->meter.domain != UtilityEnergyDomain::Electricity || c->capacity_j != capacity ||
				(c->active ? c->linked_producers != std::vector{expected_source} : !c->linked_producers.empty()) ||
				(site.state.owner != OWNER_NONE && c->owner != site.state.owner)) return false;
		}
		// Each berth retains its own once-only work epoch even though both Dock jobs share one bounded ledger consumer.
		for (const auto *job : m.GetServiceJobs()) if (job->last_work_interval > power.interval) return false;
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
		const auto jobs = site->module->GetServiceJobs();
		// Resolve every contained actual-world berth after native hulls/queues; removed hulls retain their parts for explicit cancellation.
		for (size_t berth = 0; berth < jobs.size(); ++berth) {
			const auto &job = *jobs[berth];
			if (job.remaining_ticks == 0) continue;
			if (job.craft.generation >= GetNextSpaceVehicleGeneration()) return false;
			const auto *craft = ResolveSpaceCraft(job.craft);
			// A surviving incarnation must retain its original owner, paid native role and exact unchanged service serial.
			if (craft != nullptr && (craft->owner != site->state.owner || !craft->manifest || craft->manifest->service_ticks != 0 || craft->manifest->services_completed != job.services_at_start)) return false;
			if (craft != nullptr) {
				const auto model = GetSpaceVehicleModel(*craft);
				// Current known bills must match this exact paid role; genuine unknown legacy receipts cannot be converted into heavy rights by a surviving or reused hull.
				if (!GetSpaceServiceTuning(model) || (job.model == UINT16_MAX ? model == SPACE_HEAVY_MINER_MODEL : model != job.model) || craft->construction.placement != SpacePlacement::Orbital || !IsSupportedSpaceVehicleContentVersion(craft->construction.content_version, model)) return false;
				// The factory-only heavy's retained contained job requires exhausted allowance, no outstanding descent and actual holding pose throughout saved work; legacy probe policy remains unchanged.
				if (model == SPACE_HEAVY_MINER_MODEL && (craft->manifest->returns_remaining != 0 || craft->manifest->pending_returns != 0 || craft->mission.phase != SpaceMissionPhase::Holding)) return false;
				// Shared admission is mandatory for both new heavy Core work and all Dock work; historical Core jobs retain their original precise-coordinate contract.
				if (site->state.kind == SpaceBuildKind::Dock || model == SPACE_HEAVY_MINER_MODEL) {
					const auto admitted = GetSpaceStructureBerth(*craft, *site);
					if (!admitted || *admitted != berth) return false;
				} else if (craft->position != GetSpaceStructureApproach(site->state)) return false;
			}
		}
	}
	return true;
}
