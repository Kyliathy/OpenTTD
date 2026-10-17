/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_manufacturing.cpp Paid native requests, whole-recipe reservation, common-grant work and once-only materialization.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups7/8/8BB/8BC
 */
#include "stdafx.h"
#include "orbital_manufacturing.h"
#include "space_structure.h"
#include "space_structure_cmd.h"
#include "command_func.h"
#include "space_approach.h"
#include "orbital_service.h"
#include "hub_cargo.h"
#include "company_base.h"
#include "company_func.h"
#include "multilayer_content.h"
#include "core/backup_type.hpp"
#include "openttd.h"
#include "transport_tube.h"
#include "timer/timer_game_tick.h"
#include "gfx_func.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_replacement.h"
#include "engine_base.h"
#include "vehicle_func.h"
#include "group.h"
#include "safeguards.h"

/** @param model Immutable requested model. @return Its complete supported orbital recipe, without registering any Engine or hull. */
static const SpaceConstructionDefinition *ManufactureDefinition(uint16_t model)
{
	const auto *definition = GetSpaceVehicleConstruction(model);
	// Only the profile's declared orbital vehicle roles may enter this facility; static recipes, rockets and incomplete extra-input lists cannot become manufacturing orders.
	if (definition == nullptr || !CanCommissionSpaceVehicle(model, MultiLayerCommissioningFacility::OrbitalAssembly, true) ||
		definition->inputs.empty() || definition->inputs.size() > MAX_ORBITAL_MANUFACTURE_INPUTS) return nullptr;
	return definition;
}

/** Count the actual split allocation needed by one existing physical source, following the native transfer's next-hop order.
 * @param source Actual station-like stock owner. @param quantity Complete positive recipe quantity.
 * @return Zero/one split, or absence when the complete quantity cannot move.
 */
static std::optional<uint> ManufactureSplits(const StationCargoList &source, uint quantity)
{
	// Unavailable or reserved quantities cannot be promised again to a second orbital order.
	if (quantity == 0 || source.AvailableCount() < quantity) return std::nullopt;
	// Native packet order determines the final boundary; the whole four-label batch must reserve every required split before its first mutation.
	for (const auto &[hop, packets] : *source.Packets()) {
		(void)hop;
		for (const auto *packet : packets) {
			if (quantity < packet->Count()) return 1;
			quantity -= packet->Count();
			if (quantity == 0) return 0;
		}
	}
	return std::nullopt;
}

/** @param stock Actual physical cargo owner. @return Its bounded saved packet-reference count, including every next-hop bucket. */
static size_t ManufacturePacketCount(const StationCargoList &stock)
{
	size_t count = 0;
	// Quantity headroom alone cannot admit an evacuation into an already fragmented destination.
	for (const auto &[hop, packets] : *stock.Packets()) { (void)hop; count += packets.size(); }
	return count;
}

/** @param site Actual Assembly. @param job Proposed or contained paid request. @return Its exact currently usable adjacent stock owner, or nullptr. */
static SpaceStructure *ManufactureSource(const SpaceStructure &site, const OrbitalManufactureState &job)
{
	auto *source = ResolveSpaceStructure(job.source);
	// Cargo comes from a real owned adjacent inventory, never the Assembly's zero-capacity stock or a reused source incarnation.
	if (source == nullptr || source->state.owner != site.state.owner ||
		(source->state.kind != SpaceBuildKind::Core && source->state.kind != SpaceBuildKind::Storage) ||
		!IsOrbitalModuleOperational(*source) || !AreOrbitalModuleFootprintsAdjacent(site.state, source->state)) return nullptr;
	return source;
}

/** @param site Actual Assembly. @param job Proposed or queued request. @return Whether every material, physical slot and original electrical connection can admit reservation. */
static bool ReadyManufactureFacility(const SpaceStructure &site, const OrbitalManufactureState &job)
{
	const auto *definition = ManufactureDefinition(job.model);
	// A complete paid facility and its actual owned company/technology must survive through the normal allocation boundary.
	if (site.state.kind != SpaceBuildKind::Assembly || !IsOrbitalModuleOperational(site) || !Company::IsValidID(site.state.owner) ||
		definition == nullptr || TimerGameCalendar::year < definition->introduction_year || job.berth != 0 || !IsSpaceStructureManufactureBerthFree(site)) return false;
	const auto *source = ManufactureSource(site, job); const auto *panel = GetOrbitalServicePowerSite(site);
	// A configured but unavailable/retired panel supplies no admission grant, and an unrelated nearby source cannot substitute for it.
	if (source == nullptr || panel == nullptr || GetOrbitalModulePanelRate(*panel) == 0) return false;
	const auto *consumer = GetUtilityPowerConsumer(site.module->consumer);
	// This exact dedicated750J work identity belongs to the Assembly; it cannot spend the Core's construction consumer or another work family's allowance.
	if (consumer == nullptr || !consumer->active || consumer->owner != site.state.owner || consumer->effect != UtilityConsumerEffect::AssemblyWork ||
		consumer->capacity_j != GetMultiLayerSpaceBudget().work_j_per_tick || consumer->linked_producers != std::vector{panel->module->producer}) return false;
	return true;
}

/** @param site Actual paid Assembly. @param job Exact queued request. @return Whether all labels can enter one original reservation without overbooking packet splits. */
static bool ReadyManufactureReservation(const SpaceStructure &site, const OrbitalManufactureState &job)
{
	if (!ReadyManufactureFacility(site, job)) return false;
	const auto *source = ManufactureSource(site, job); const auto *definition = ManufactureDefinition(job.model);
	uint splits = 0;
	// Preflight every declared label before moving any packet, including heavy components/electronics and their aggregate split-pool demand.
	for (const auto &input : definition->inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		if (!IsValidCargoType(cargo)) return false;
		const auto required = ManufactureSplits(source->module->stock[cargo], input.quantity);
		if (!required) return false;
		splits += *required;
	}
	return CargoPacket::CanAllocateItem(splits);
}

/** @param job Actual contained batch. @return Whether every declared original label is physically complete and no unused slot holds hidden material. */
static bool HasManufactureInputs(const OrbitalManufactureJob &job)
{
	const auto *definition = ManufactureDefinition(job.model);
	if (definition == nullptr) return false;
	// Work and completion require the same complete bill; a missing late component cannot spend another grant or consume earlier labels.
	for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) {
		const uint expected = input < definition->inputs.size() ? definition->inputs[input].quantity : 0;
		if (job.inputs[input].AvailableCount() != expected) return false;
	}
	return true;
}

/** Share the original complete admission gate between ordinary typed manufacture and an explicit old-hull replacement context.
 * @param flags Actual estimate/execution; QueryCost never publishes a request.
 * @param site Actual resolved Assembly incarnation.
 * @param source Exact existing adjacent inventory incarnation.
 * @param model Canonical target recipe.
 * @param replacement Optional genuine old-hull context; ordinary requests cannot acknowledge another completed receipt implicitly.
 * @return Full new physical capital cost or an unchanged refusal.
 */
static CommandCost QueueManufacture(DoCommandFlags flags, SpaceStructure &site, SpaceStructureRef source, uint16_t model, const SpaceReplacementIntent *replacement)
{
	const auto *definition = ManufactureDefinition(model);
	// A synchronized request creates one paid job, never an immediate placeholder hull or a second request sharing this facility's slot.
	if (definition == nullptr || !Company::IsValidID(_current_company) || site.state.owner != _current_company ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		!site.module || site.module->manufacture_serial == UINT64_MAX) return CMD_ERROR;
	if (site.module->manufacture) {
		const auto &previous = *site.module->manufacture;
		const auto *old = replacement == nullptr ? nullptr : ResolveSpaceCraft(replacement->source);
		// Automatic replacement may acknowledge only its own real already completed birthplace receipt. Every unrelated or unfinished job keeps the existing explicit clear/cancel owner.
		if (old == nullptr || previous.phase != OrbitalManufacturePhase::Complete || previous.product != replacement->source ||
			old->construction.assembly != GetSpaceStructureReference(site) || old->construction.assembly_serial != previous.serial) return CMD_ERROR;
		// A completed receipt must own no physical input; never discard an unexpected packet while superseding metadata for the next paid request.
		for (const auto &input : previous.inputs) if (input.TotalCount() != 0) return CMD_ERROR;
	}
	if (replacement != nullptr) {
		const SpaceReplacementSite handover{GetSpaceStructureReference(site), source, {}};
		// The original producer validates the explicit old-hull context too; a direct consumer cannot manufacture a second pending retirement or acknowledge a foreign source.
		if (HasPendingSpaceReplacement(replacement->source) || !PrepareSpaceReplacement(*replacement, model, site.state.owner, handover)) return CMD_ERROR;
	}
	if (flags.Test(DoCommandFlag::QueryCost)) flags.Reset(DoCommandFlag::Execute);
	OrbitalManufactureState proposed;
	proposed.model = model; proposed.serial = site.module->manufacture_serial + 1;
	proposed.source = source; proposed.capital_paid = definition->capital;
	if (replacement != nullptr) proposed.replacement = *replacement;
	if (!ReadyManufactureReservation(site, proposed)) return CMD_ERROR;
	CommandCost cost(ExpensesType::NewVehicles, definition->capital);
	if (!CheckCompanyHasMoney(cost)) return cost;
	if (flags.Test(DoCommandFlag::Execute)) {
		auto job = std::make_unique<OrbitalManufactureJob>(); static_cast<OrbitalManufactureState &>(*job) = proposed;
		site.module->manufacture = std::move(job); site.module->manufacture_serial = proposed.serial;
		MarkWholeScreenDirty();
	}
	return cost;
}

/** @copydoc CmdQueueOrbitalManufacture */
CommandCost CmdQueueOrbitalManufacture(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, SpaceStructureID source_id, uint64_t source_generation, uint16_t model)
{
	auto *site = ResolveSpaceStructure({id, generation});
	if (site == nullptr) return CMD_ERROR;
	return QueueManufacture(flags, *site, {source_id, source_generation}, model, nullptr);
}

/** @copydoc QueueOrbitalReplacement */
CommandCost QueueOrbitalReplacement(DoCommandFlags flags, SpaceStructure &factory, SpaceStructure &inventory, uint16_t model, const SpaceReplacementIntent &replacement)
{
	return QueueManufacture(flags, factory, GetSpaceStructureReference(inventory), model, &replacement);
}

/** @copydoc AppendOrbitalManufacturingRequests */
void AppendOrbitalManufacturingRequests(std::vector<UtilityPowerRequest> &requests)
{
	// Gathering is a normal common-phase query; idle facilities and paused/editor inspection must not reserve speculative electricity.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || GetTubePowerPreparedTick() != TimerGameTick::counter) return;
	// Each actual paid queued job contributes at most its one original bounded consumer, in native facility order.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		const auto *job = site->module->manufacture.get();
		if (job == nullptr) continue;
		// Only the original queued reservation or an unfinished physically complete work batch may demand its one bounded grant; blocked/finished materialization requests none.
		const bool queued = job->phase == OrbitalManufacturePhase::Queued;
		const bool working = job->phase == OrbitalManufacturePhase::Reserved || job->phase == OrbitalManufacturePhase::Working;
		if (queued ? !ReadyManufactureReservation(*site, *job) : (!working || !ReadyManufactureFacility(*site, *job) || !HasManufactureInputs(*job))) continue;
		requests.push_back({site->module->consumer, GetMultiLayerSpaceBudget().work_j_per_tick});
	}
}

/** @copydoc TickOrbitalManufacturingReservations */
void TickOrbitalManufacturingReservations()
{
	// Loading, repeated paused callbacks and old prepared epochs cannot turn a promise into actual packet ownership.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || GetTubePowerPreparedTick() != TimerGameTick::counter) return;
	// Deterministic facility order revalidates competing stock after earlier commits; only one complete batch can claim each physical quantity.
	for (auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		auto *job = site->module->manufacture.get();
		if (job == nullptr || job->phase != OrbitalManufacturePhase::Queued) continue;
		if (!ReadyManufactureReservation(*site, *job)) continue;
		AutoRestoreBackup actor(_current_company, site->state.owner);
		const auto grant = GetUtilityAllocation(site->module->consumer);
		// Materials remain at their source until this actual normal allocation admits the whole work quantum; observation consumes no energy or work.
		if (!grant || grant->granted_energy_j < GetMultiLayerSpaceBudget().work_j_per_tick || GetUtilityPowerInterval() == 0) continue;
		auto &source = *ManufactureSource(*site, *job); const auto &definition = *ManufactureDefinition(job->model);
		// The complete split-pool and quantity preflight above makes every move infallible within this single-threaded native opportunity.
		for (size_t input = 0; input < definition.inputs.size(); ++input) {
			const auto &required = definition.inputs[input]; const auto cargo = GetCargoTypeByLabel(required.label);
			const bool moved = HubCargoTransfer::Move(source.module->stock[cargo], job->inputs[input], required.quantity);
			assert(moved); (void)moved;
		}
		job->phase = OrbitalManufacturePhase::Reserved; job->reservation_interval = GetUtilityPowerInterval();
		MarkWholeScreenDirty();
	}
}

/** One exact native opportunity participates in the existing common energy transaction; physical work never writes the ledger privately. */
class OrbitalManufacturingStep final : public UtilityMutationParticipant {
	SpaceStructure &site;
	OrbitalManufactureJob &job;
	uint32_t previous;
public:
	/** @param site Original paid factory. @param job Its exact contained recipe batch, including already paid work. */
	OrbitalManufacturingStep(SpaceStructure &site, OrbitalManufactureJob &job) : site(site), job(job), previous(job.work_ticks) {}
	/** @return The original dedicated AssemblyWork effect, without a generic bonus or substitute consumer. */
	std::optional<UtilityConsumerEffect> EffectKind() const override { return UtilityConsumerEffect::AssemblyWork; }
	/** @param proposal Exact common consume proposal. @return Whether the original physical batch/epoch still admits precisely one whole opportunity, without utility API reentry. */
	bool Validate(const UtilityMutationProposal &proposal) const override
	{
		const auto *definition = ManufactureDefinition(this->job.model);
		return _game_mode == GameMode::Normal && !_pause_mode.Any() && this->site.module && this->site.module->manufacture.get() == &this->job &&
			Company::IsValidID(this->site.state.owner) && proposal.owner == this->site.state.owner && IsOrbitalModuleOperational(this->site) &&
			(this->job.phase == OrbitalManufacturePhase::Reserved || this->job.phase == OrbitalManufacturePhase::Working) &&
			definition != nullptr && this->job.work_ticks == this->previous && this->previous < definition->work_ticks && HasManufactureInputs(this->job) &&
			proposal.grant.consumer == this->site.module->consumer && proposal.quantity_j == GetMultiLayerSpaceBudget().work_j_per_tick &&
			this->job.reservation_interval < proposal.grant.interval && this->job.last_work_interval < proposal.grant.interval &&
			GetTubePowerPreparedTick() == TimerGameTick::counter && IsSpaceStructureManufactureBerthFree(this->site);
	}
	/** @param proposal Already admitted original energy debit. @return True after advancing this exact saved work receipt once, with no allocation or ledger reentry. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override
	{
		++this->job.work_ticks; this->job.last_work_interval = proposal.grant.interval; this->job.work_energy_j += proposal.quantity_j;
		this->job.phase = this->job.work_ticks == ManufactureDefinition(this->job.model)->work_ticks ? OrbitalManufacturePhase::AwaitingMaterialization : OrbitalManufacturePhase::Working;
		return true;
	}
};

/** Prepare every real capacity before the sole final material milestone and native-hull commit.
 * @param site Original Assembly whose actual work is already fully paid.
 * @param job Its original complete physical recipe batch; blocked capacity retains work and every input for retry.
 * @return Whether one native product was created and bound to its original berth.
 */
static bool MaterializeManufacture(SpaceStructure &site, OrbitalManufactureJob &job)
{
	const auto *recipe = ManufactureDefinition(job.model);
	if (recipe == nullptr) return false;
	// A finished receipt is never executable work; all model, material, physical slot and native capacity refusals precede the first mutation.
	if (job.phase != OrbitalManufacturePhase::AwaitingMaterialization || job.work_ticks != recipe->work_ticks ||
		job.work_energy_j != uint64_t{job.work_ticks} * GetMultiLayerSpaceBudget().work_j_per_tick ||
		!ReadyManufactureFacility(site, job) || !HasManufactureInputs(job) ||
		GetSpaceVehicleCount() >= MAX_SPACE_VEHICLES || GetNextSpaceVehicleGeneration() == UINT64_MAX || !Vehicle::CanAllocateItem()) return false;
	AutoRestoreBackup actor(_current_company, site.state.owner);
	const auto unit = GetFreeUnitNumber(VehicleType::Space);
	// A missing Engine and an exhausted company unit range retain the same completed job; estimates/preflight never register a replacement definition or temporary hull.
	if (unit == UINT16_MAX || (!Engine::IsValidID(GetSpaceEngine(job.model)) && !Engine::CanAllocateItem())) return false;
	std::optional<SpaceCloneProductConfiguration> clone;
	std::optional<PreparedSpaceReplacement> replacement;
	if (job.replacement) {
		// Every current original holding/configuration and physical handover right must be safe before consuming the full bill or registering/allocating any new hull.
		if (job.clone) return false;
		replacement = PrepareSpaceReplacement(*job.replacement, job.model, site.state.owner, {GetSpaceStructureReference(site), job.source, {}});
		if (!replacement) return false;
	}
	if (job.clone) {
		if (!ValidateSpaceCloneIntent(*job.clone, job.model, GetNextSpaceVehicleGeneration(), GetNextSpaceStructureGeneration())) return false;
		// Prepare independent name/instruction storage before even Engine registration or hull allocation. Changed live target rights retain the original full paid batch for retry/cancellation.
		clone = PrepareSpaceCloneProductConfiguration(*job.clone, site.state.owner);
		if (!clone) return false;
	}
	if (!EnsureSpaceEngineDefinition(job.model)) return false;
	const auto &definition = *ManufactureDefinition(job.model);
	auto *craft = SpaceVehicle::Create(); const bool issued = RegisterSpaceVehicleIdentity(*craft); assert(issued); (void)issued;
	craft->owner = site.state.owner; craft->engine_type = GetSpaceEngine(job.model); craft->unitnumber = unit;
	Company::Get(craft->owner)->freeunits[VehicleType::Space].UseID(unit);
	craft->cargo_type = craft->GetEngine()->GetDefaultCargoType(); craft->cargo_cap = GetSpaceFreightCapacity(job.model, craft->cargo_type);
	craft->vcache.cached_cargo_age_period = Ticks::CARGO_AGING_TICKS; craft->value = definition.capital;
	craft->build_year = TimerGameCalendar::year; craft->max_age = craft->GetEngine()->GetLifeLengthInDays(); craft->reliability = 0xFFFF;
	craft->position = craft->home = *GetSpaceStructureApproachSlot(site, job.berth); craft->mission.phase = SpaceMissionPhase::Holding;
	craft->vehstatus.Reset({VehState::Stopped, VehState::Hidden});
	craft->construction.content_version = MULTILAYER_SPACE_CONTENT_VERSION; craft->construction.model = job.model;
	craft->construction.capital_paid = job.capital_paid; craft->construction.deployed = true;
	craft->construction.assembly = GetSpaceStructureReference(site); craft->construction.assembly_serial = job.serial;
	GetOrCreateSpaceManifest(*craft).returns_remaining = GetMultiLayerSpaceBudget().returns_per_service;
	if (clone) ApplySpaceCloneProductConfiguration(*craft, std::move(*clone)); // Independent instructions start stopped; the actual factory berth is admitted below, never copied from the original source.
	// Whole original containers need no split allocation: every label becomes this exact hull's embodied recipe in the same prepared action, without freight income or another capital debit.
	for (size_t input = 0; input < definition.inputs.size(); ++input) {
		const auto quantity = definition.inputs[input].quantity;
		const bool consumed = HubCargoTransfer::Consume(job.inputs[input], quantity); assert(consumed); (void)consumed;
		job.consumed[input] = static_cast<uint16_t>(quantity);
	}
	job.phase = OrbitalManufacturePhase::Complete; job.product = GetSpaceCraftReference(*craft);
	const bool bound = BindSpaceManufactureBerth(*craft, site); assert(bound); (void)bound;
	GroupStatistics::CountEngine(craft, 1); GroupStatistics::CountVehicle(craft, 1);
	if (replacement) CommitSpaceReplacement(*job.replacement, *craft, std::move(*replacement)); // Original work/material owners publish one real product before native old/new settlement; no estimate hull exists.
	craft->UpdateSpacePosition(); craft->MarkDirty(); MarkWholeScreenDirty(); return true;
}

/** @copydoc TickOrbitalManufacturingWork */
void TickOrbitalManufacturingWork()
{
	// Paused inspection, loading and old common preparation cannot spend work, material or a new native identity.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || GetTubePowerPreparedTick() != TimerGameTick::counter) return;
	// Native factory order shares the existing allocation and pool capacity deterministically; each job retains its own exact original epoch and serial.
	for (auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		auto *job = site->module->manufacture.get();
		if (job == nullptr) continue;
		const bool working = job->phase == OrbitalManufacturePhase::Reserved || job->phase == OrbitalManufacturePhase::Working;
		// The reservation opportunity performs no work. Repeated callbacks cannot spend the same grant or restart completed progress.
		if (working && job->reservation_interval < GetUtilityPowerInterval() && job->last_work_interval < GetUtilityPowerInterval() &&
			ReadyManufactureFacility(*site, *job) && HasManufactureInputs(*job)) {
			AutoRestoreBackup actor(_current_company, site->state.owner);
			OrbitalManufacturingStep step(*site, *job);
			const auto proposal = PrepareUtilityMutation(UtilityMutationKind::Consume, {site->module->consumer, GetUtilityPowerInterval()}, GetMultiLayerSpaceBudget().work_j_per_tick);
			CommitUtilityMutation(proposal, &step);
		}
		if (job->phase == OrbitalManufacturePhase::AwaitingMaterialization) MaterializeManufacture(*site, *job);
	}
}

/** @copydoc CmdCancelOrbitalManufacture */
CommandCost CmdCancelOrbitalManufacture(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, SpaceStructureID destination_id, uint64_t destination_generation)
{
	if (flags.Test(DoCommandFlag::QueryCost)) flags.Reset(DoCommandFlag::Execute); // A combined execute/quote must leave the original packets, source running intent and contained receipt untouched.
	auto *site = ResolveSpaceStructure({id, generation});
	// Cancellation follows this actual company's retained job even after power or its construction dependency becomes unavailable.
	if (site == nullptr || !Company::IsValidID(_current_company) || site->state.owner != _current_company ||
		!site->module || !site->module->manufacture) return CMD_ERROR;
	auto &job = *site->module->manufacture; const auto &definition = *ManufactureDefinition(job.model);
	SpaceStructure *destination = nullptr;
	// Unfinished work owns the full unconsumed bill; a completed job is only acknowledged, leaving its actual product and already spent recipe untouched.
	if (job.phase != OrbitalManufacturePhase::Queued && job.phase != OrbitalManufacturePhase::Complete) {
		OrbitalManufactureState target; target.source = {destination_id, destination_generation}; destination = ManufactureSource(*site, target);
		if (destination == nullptr) return CMD_ERROR;
		// Return the whole unconsumed batch to one explicit safe inventory; capacity/packet fragmentation is checked across every declared label before mutation.
		for (size_t input = 0; input < definition.inputs.size(); ++input) {
			const auto cargo = GetCargoTypeByLabel(definition.inputs[input].label); const auto &stock = destination->module->stock[cargo];
			if (uint64_t{stock.TotalCount()} + job.inputs[input].TotalCount() > GetOrbitalModuleCargoCapacity(*destination, cargo) ||
				ManufacturePacketCount(stock) + ManufacturePacketCount(job.inputs[input]) > 64) return CMD_ERROR;
		}
	}
	if (flags.Test(DoCommandFlag::Execute)) {
		if (destination != nullptr) {
			// Returning complete containers needs no split allocation, income callback or replacement cargo; original provenance and feeder shares survive.
			for (size_t input = 0; input < definition.inputs.size(); ++input) {
				const auto cargo = GetCargoTypeByLabel(definition.inputs[input].label);
				const bool moved = HubCargoTransfer::Move(job.inputs[input], destination->module->stock[cargo], job.inputs[input].TotalCount());
				assert(moved); (void)moved;
			}
		}
		if (job.replacement) RestoreCancelledSpaceReplacement(*job.replacement, job.model, site->state.owner, {GetSpaceStructureReference(*site), job.source, {}});
		site->module->manufacture.reset(); MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc ValidateOrbitalManufacturingRecords */
bool ValidateOrbitalManufacturingRecords(std::span<const SpaceStructureRecord> rows, uint64_t interval)
{
	// Every manufacturing owner must resolve within this exact native snapshot; active-world stocks and reused numeric IDs cannot legitimize it.
	for (const auto &site : rows) {
		if (!site.module) continue;
		const auto &module = *site.module;
		if (site.state.kind != SpaceBuildKind::Assembly) {
			// Other module roles cannot smuggle an unpaid recipe or manufacturing issuance through their common contained facility transport.
			if (module.manufacture_serial != 0 || module.manufacture) return false;
			continue;
		}
		if (!module.manufacture) continue;
		const auto &job = *module.manufacture; const auto *definition = ManufactureDefinition(job.model);
		// The current order owns exactly its issued serial/capital and one real original connection; neither queued nor reserved states may fabricate work/hulls.
		if (definition == nullptr || job.phase >= OrbitalManufacturePhase::End || job.serial == 0 || job.serial != module.manufacture_serial ||
			job.source.generation == 0 || job.source.structure >= SpaceStructureID::End() ||
			job.capital_paid != definition->capital || job.berth != 0 || module.activation_ticks != 0 || site.state.phase != SpaceStructurePhase::FrameComplete ||
			module.consumer == UtilityConsumerID{UINT32_MAX} || module.utility_source == SpaceStructureRef{} ||
			(job.phase == OrbitalManufacturePhase::Queued ? job.reservation_interval != 0 : job.reservation_interval == 0) || job.reservation_interval > interval) return false;
		const bool complete = job.phase == OrbitalManufacturePhase::Complete;
		const bool awaiting = job.phase == OrbitalManufacturePhase::AwaitingMaterialization;
		const bool working = job.phase == OrbitalManufacturePhase::Working;
		// Work is an exact receipt for actually consumed common energy, with one monotonic post-reservation epoch; no imported phase may fabricate free progress or replay a completion.
		if (job.work_ticks > definition->work_ticks || job.work_energy_j != uint64_t{job.work_ticks} * GetMultiLayerSpaceBudget().work_j_per_tick ||
			(job.work_ticks == 0 ? job.last_work_interval != 0 : job.last_work_interval <= job.reservation_interval) || job.last_work_interval > interval ||
			((complete || awaiting) ? job.work_ticks != definition->work_ticks : (working ? (job.work_ticks == 0 || job.work_ticks >= definition->work_ticks) : job.work_ticks != 0)) ||
			(complete ? (job.product.vehicle >= VehicleID::End() || job.product.generation == 0) : job.product != SpaceCraftRef{})) return false;
		/** @param candidate Same-file physical stock owner. @return Whether its original issued incarnation matches this job. */
		const auto source = std::ranges::find_if(rows, [&](const auto &candidate) { return candidate.ref == job.source; });
		// Company loss retains both actual owners without operating rights; takeover must transfer both, while absent/foreign sources always refuse.
		if (source == rows.end() || !source->module || source->state.owner != site.state.owner || source->module->activation_ticks != 0 ||
			source->state.phase != SpaceStructurePhase::FrameComplete ||
			(source->state.kind != SpaceBuildKind::Core && source->state.kind != SpaceBuildKind::Storage) ||
			!AreOrbitalModuleFootprintsAdjacent(site.state, source->state)) return false;
		// Queued/completed receipts own no packet; all unfinished paid-work phases retain the complete bill until the one final native-hull commit.
		for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) {
			const auto quantity = input < definition->inputs.size() ? definition->inputs[input].quantity : 0;
			const auto expected = job.phase != OrbitalManufacturePhase::Queued && !complete ? quantity : 0;
			if (job.quantities[input] != expected || job.inputs[input].size() > expected || (expected == 0) != job.inputs[input].empty() ||
				job.consumed[input] != (complete ? quantity : 0)) return false;
		}
	}
	return true;
}

/** @copydoc ValidateOrbitalManufacturingProductionLinks */
bool ValidateOrbitalManufacturingProductionLinks(std::span<const SpaceStructureRecord> rows, std::span<const SpaceProductionRecord> products,
	uint64_t next_structure, uint64_t next_vehicle)
{
	std::set<std::tuple<uint16_t, uint64_t, uint64_t>> issued;
	// A single original factory/job can embody its paid bill in only one surviving native hull, even after the facility or an earlier product has been removed.
	for (const auto &product : products) {
		const auto &state = product.state;
		if (state.assembly == SpaceStructureRef{}) continue;
		if (state.assembly.generation >= next_structure || !issued.emplace(state.assembly.structure.base(), state.assembly.generation, state.assembly_serial).second) return false;
		/** @param row Same-snapshot factory. @return Whether this is the original issued incarnation, never a reused numeric ID. */
		const auto factory = std::ranges::find_if(rows, [&](const auto &row) { return row.ref == state.assembly; });
		if (factory == rows.end()) continue; // Actual retired factories do not invalidate an already paid surviving product.
		// A surviving factory must have issued this serial; a pending batch cannot share its materials with a forged already completed product.
		if (factory->state.kind != SpaceBuildKind::Assembly || !factory->module || state.assembly_serial > factory->module->manufacture_serial ||
			(factory->module->manufacture && factory->module->manufacture->serial == state.assembly_serial &&
			(factory->module->manufacture->phase != OrbitalManufacturePhase::Complete || factory->module->manufacture->product != product.craft))) return false;
	}
	// A retained completion receipt must identify its one actual product when it survives; sale/deletion may leave the original dated receipt without permitting another materialization.
	for (const auto &row : rows) {
		if (!row.module || !row.module->manufacture) continue;
		const auto &job = *row.module->manufacture;
		// Pending and completed requests retain bounded historical clone intent against this same file's issuance, without resolving a removed/changed original hull or current-world preview owner.
		if (job.clone && !ValidateSpaceCloneIntent(*job.clone, job.model, next_vehicle, next_structure)) return false;
		if (job.clone) {
			/** @param product Same-file paid hull receipt. @return Whether this is the captured source incarnation, never another hull reusing its numeric ID. */
			const auto source = std::ranges::find_if(products, [&](const auto &product) { return product.craft == job.clone->source; });
			const auto *recipe = ManufactureDefinition(job.model);
			// Source ownership and runtime cargo may legitimately change; a surviving original incarnation retains its immutable paid canonical model/receipt.
			if (recipe == nullptr || (source != products.end() && (source->state.model != job.model || !IsSupportedSpaceVehicleContentVersion(source->state.content_version, source->state.model) || source->state.capital_paid != recipe->capital))) return false;
		}
		if (job.phase != OrbitalManufacturePhase::Complete) continue;
		// An actual new product must postdate its captured source, even when its pool ID legitimately reuses that retired source's numeric slot.
		if (job.clone && job.clone->source.generation >= job.product.generation) return false;
		if (job.product.generation >= next_vehicle) return false;
		/** @param product Same-file paid native receipt. @return Whether it belongs to this exact completed hull incarnation. */
		const auto product = std::ranges::find_if(products, [&](const auto &product) { return product.craft == job.product; });
		if (product != products.end() && (product->state.assembly != row.ref || product->state.assembly_serial != job.serial || product->state.model != job.model)) return false;
	}
	return ValidateSpaceReplacementLinks(rows, products, next_vehicle);
}
