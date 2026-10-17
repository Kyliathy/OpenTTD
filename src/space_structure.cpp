/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_structure.cpp Native paid sites, Core activation and exclusive orbital cargo owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups1-8BC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10B; native processor buffers resolve actual configured cargo labels.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C; processing work/connection/history follows exact native activation, freight and removal owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12; haulers use explicit precise module exchanges without orbital CargoDist endpoints.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group6 shared body-strip exclusion.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C; an actual heavy orbital return spends its original manifest allowance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC; immutable contained-job bills bound real reserved packets and survive sender loss.
 */
#include "stdafx.h"
#include "space_structure.h"
#include "orbital_manufacturing.h"
#include "orbital_recipe.h"
#include "multilayer_content.h"
#include "space_transition.h"
#include "space_approach.h"
#include "space_structure_cmd.h"
#include "space_vehicle.h"
#include "space_mining_tuning.h"
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
#include "core/backup_type.hpp"
#include "timer/timer_game_tick.h"
#include <bit>
#include <set>
#include "safeguards.h"

SpaceStructurePool _space_structure_pool("SpaceStructure");
INSTANTIATE_POOL_METHODS(SpaceStructure)
static uint64_t next_generation = 1;
static SpaceStructureRetirementHistory retirement_history;
/** @copydoc GetSpaceStructureRetirementHistory */
const SpaceStructureRetirementHistory &GetSpaceStructureRetirementHistory() { return retirement_history; }
/** @copydoc InstallLoadedSpaceStructureProcessingHistory */
bool InstallLoadedSpaceStructureProcessingHistory(const SpaceStructureRetirementHistory &history)
{
	auto expected = retirement_history; expected.processing_batches = history.processing_batches; expected.processing_discarded_paid_j = history.processing_discarded_paid_j;
	if (expected != history) return false;
	retirement_history = history; return true;
}
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
void ResetSpaceStructures() { next_generation = 1; retirement_history = {}; ResetSpaceStructureLoadStaging(); ResetOrbitalRecipeLoadStaging(); ResetOrbitalConstructionTick(); }

/** Preflight all native spent-receipt additions before any physical or shared utility owner changes.
 * @param site Exact native owner being removed once.
 * @return Updated same-authority aggregate, or absence on any overflow.
 */
static std::optional<SpaceStructureRetirementHistory> PlanSpaceStructureRetirement(const SpaceStructure &site)
{
	if (retirement_history.structures_removed == UINT64_MAX) return std::nullopt;
	auto next = retirement_history; ++next.structures_removed;
	// Record each already consumed material independently; unconsumed packets must have left through explicit physical evacuation.
	for (uint i = 0; i < next.consumed.size(); ++i) {
		if (site.job.consumed[i] > UINT64_MAX - next.consumed[i]) return std::nullopt;
		next.consumed[i] += site.job.consumed[i];
	}
	const uint64_t services = site.module ? site.module->services_completed : 0;
	if (services > UINT64_MAX - next.services_completed) return std::nullopt;
	next.services_completed += services;
	// Processing spent receipts follow their original physical module into the existing bounded retirement authority, with complete overflow admission before deletion.
	if (site.module && site.module->processing) {
		if (!ValidateOrbitalRecipes()) return std::nullopt;
		const auto &processing = *site.module->processing;
		// Retain each complete original recipe count under the same structure authority; removing its physical owner never erases consumed materials, residue or actual spent electricity.
		for (const auto &[id, batches] : processing.batches) {
			if (batches == 0) continue;
			auto &total = next.processing_batches[id]; if (batches > UINT64_MAX - total) return std::nullopt;
			total += batches;
		}
		if (processing.discarded_paid_j > UINT64_MAX - next.processing_discarded_paid_j) return std::nullopt;
		next.processing_discarded_paid_j += processing.discarded_paid_j;
		if (!GetOrbitalRecipeRetirementEnergy(next)) return std::nullopt;
	}
	return next;
}
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
		for (auto *job : this->module->GetServiceJobs()) {
			for (auto &slot : job->inputs) {
				if (SpaceStructure::CleaningPool()) slot.OnCleanPool();
				else assert(slot.TotalCount() == 0);
			}
		}
		// A manufacturing batch has the same teardown rule: only world cleanup may discard its native packet references; ordinary removal requires explicit return.
		if (this->module->manufacture) for (auto &slot : this->module->manufacture->inputs) {
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
	const auto &tuning = GetMultiLayerSpaceTuning();
	// The complete footprint and south approach must stay in an outer static region; the middle strip reserves every moving-body envelope.
	return uint32_t{s.y} + d->height < height * tuning.travel_start_quarters / 4 ||
		s.y >= height * tuning.travel_end_quarters / 4;
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

/** Check accountable physical occupancy without assigning a berth or moving a hull.
 * @param site Actual native footprint and its whole reserved south approach cell.
 * @return Whether any actual Space hull remains within the retained site/approach.
 */
static bool HasOrbitalRemovalOccupant(const SpaceStructure &site)
{
	if (!ReservesSite(site)) return false;
	const auto &definition = *GetSpaceConstructionDefinition(site.state.kind);
	const int64_t left = int64_t{site.state.x} * SPACE_Q16, top = int64_t{site.state.y} * SPACE_Q16;
	const int64_t right = left + int64_t{definition.width} * SPACE_Q16, bottom = top + int64_t{definition.height} * SPACE_Q16;
	// Check actual physical Space positions, including a visiting rocket; a removed queue alone cannot vacate an occupied module.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		if (GetSpaceVehicleLayer(*craft) != MapLayer::Space) continue;
		const auto &point = craft->position;
		const bool footprint = point.x >= left && point.x < right && point.y >= top && point.y < bottom;
		const bool approach = point.x >= left && point.x < left + SPACE_Q16 && point.y >= bottom && point.y < bottom + SPACE_Q16;
		if (footprint || approach) return true;
	}
	return false;
}

/** @param site Actual native site. @return Whether any retained packet/job/hull would lose its sole completed-Core dependency. */
static bool HasOrbitalRemovalAccountableItem(const SpaceStructure &site)
{
	// A paid active construction job or its physical packets cannot be discarded when its original assembly source disappears.
	if (site.job.inputs[0].TotalCount() != 0 || site.job.inputs[1].TotalCount() != 0 ||
		(site.state.phase == SpaceStructurePhase::Blueprint && (site.job.work_ticks != 0 || site.job.work_fraction != 0))) return true;
	if (site.module) {
		if (site.module->manufacture) return true; // Even a packet-free queued order retains paid capital and its original construction dependency.
		if (site.module->processing) {
			const auto &processing = *site.module->processing;
			// Enabled/unfinished processing retains the same completed-Core dependency even after a hauler emptied its input compartment.
			if (processing.enabled || processing.unpowered_work_ticks != 0 || (processing.electrical && processing.electrical->Capture().paid_j != 0)) return true;
		}
		// Every contained service job and stock label remains accountable independently of present power availability.
		for (const auto *job : site.module->GetServiceJobs()) {
			if (job->remaining_ticks != 0 || job->inputs[0].TotalCount() != 0 || job->inputs[1].TotalCount() != 0) return true;
		}
		for (const auto &slot : site.module->stock) if (slot.TotalCount() != 0) return true;
	}
	return HasOrbitalRemovalOccupant(site);
}

/** @param state Actual dependent geometry/owner. @param removed Exact Core being removed. @return Whether another authoritative operational Core preserves this dependent's eligibility. */
static bool HasAlternativeOrbitalCore(const SpaceStructureState &state, const SpaceStructure &removed)
{
	// Derive the surviving edge directly; temporarily deactivating the removed Core would mutate a supposedly pure estimate.
	for (const auto *core : SpaceStructure::Iterate()) {
		if (core == &removed || core->state.kind != SpaceBuildKind::Core || core->state.owner != state.owner) continue;
		if (!IsOrbitalModuleOperational(*core)) continue;
		if (AreOrbitalModuleFootprintsAdjacent(state, core->state)) return true;
	}
	return false;
}

/** Preflight or retire only this module's actual typed native utility accounts.
 * @param site Native owner, possibly an inert frame with no accounts.
 * @param execute False validates the whole set; true applies one shared lifecycle operation.
 * @return Shared authority's complete outcome; no copied ledger or energy refund is used.
 */
static UtilityPowerError RetireOrbitalModuleUtility(const SpaceStructure &site, bool execute)
{
	std::array<UtilityProducerID, 1> producers{}; size_t producer_count = 0;
	std::array<UtilityConsumerID, 3> consumers{}; size_t consumer_count = 0;
	if (site.module) {
		if (site.module->producer != UtilityProducerID{UINT32_MAX}) producers[producer_count++] = site.module->producer;
		// Native service and construction remain separate physical accounts; omit absent identities rather than issue replacements.
		for (const auto id : {site.module->consumer, site.module->assembly_consumer}) if (id != UtilityConsumerID{UINT32_MAX}) consumers[consumer_count++] = id;
		if (site.module->processing) {
			const auto id = site.module->processing->consumer;
			if (id != UtilityConsumerID{UINT32_MAX}) consumers[consumer_count++] = id;
		}
	}
	return RetireOrbitalUtilityAccounts(std::span{producers}.first(producer_count), std::span{consumers}.first(consumer_count), execute);
}

/** @copydoc ClassifyOrbitalModuleRemoval */
OrbitalRemovalBlock ClassifyOrbitalModuleRemoval(const SpaceStructure &site, CompanyID owner)
{
	// An old numeric company identity cannot dispose of unowned retained material or another company's paid module.
	if (!Company::IsValidID(owner) || site.state.owner != owner) return OrbitalRemovalBlock::InvalidOwner;
	if (site.module) {
		if (site.module->manufacture) return OrbitalRemovalBlock::Construction; // Cancel the exact paid request before deleting its facility or reserved batch.
		if (site.module->processing) {
			const auto &processing = *site.module->processing;
			// Explicit cancellation must account for unfinished physical work before demolition; empty stock alone cannot waive an enabled or paid processing order.
			if (processing.enabled || processing.unpowered_work_ticks != 0 || (processing.electrical && processing.electrical->Capture().paid_j != 0)) return OrbitalRemovalBlock::Construction;
		}
		// Explicit service cancellation must return its actual parts before demolition; a stopped or lost occupant does not waive this owner.
		for (const auto *job : site.module->GetServiceJobs()) {
			if (job->remaining_ticks != 0 || job->inputs[0].TotalCount() != 0 || job->inputs[1].TotalCount() != 0) return OrbitalRemovalBlock::Service;
		}
		for (const auto &stock : site.module->stock) if (stock.TotalCount() != 0) return OrbitalRemovalBlock::Inventory;
	}
	// Construction material and already earned active work require explicit cancellation/evacuation, never implicit disposal or a refund.
	if (site.job.inputs[0].TotalCount() != 0 || site.job.inputs[1].TotalCount() != 0 ||
		(site.state.phase == SpaceStructurePhase::Blueprint && (site.job.work_ticks != 0 || site.job.work_fraction != 0))) return OrbitalRemovalBlock::Construction;
	if (HasOrbitalRemovalOccupant(site)) return OrbitalRemovalBlock::Vehicle;
	const auto reference = GetSpaceStructureReference(site);
	// Original physical source edges cannot inherit a reused ID; disconnect the actual Dock before removing its source owner.
	for (const auto *dependent : SpaceStructure::Iterate()) {
		if (dependent == &site) continue;
		if (dependent->module != nullptr && dependent->module->utility_source == reference) return OrbitalRemovalBlock::Connection;
		// A processing source is the same original physical incarnation; remove/disconnect its actual consumer before deleting that panel.
		if (dependent->module && dependent->module->processing && dependent->module->processing->source == reference) return OrbitalRemovalBlock::Connection;
		// Original stock-source identity is part of the paid order even after full reservation; source removal must follow explicit cancellation.
		if (dependent->module && dependent->module->manufacture && dependent->module->manufacture->source == reference) return OrbitalRemovalBlock::Construction;
		// Only removing a real completed Core can invalidate an existing dependent; unfinished/cancelled frames supplied no eligibility.
		if (site.state.kind != SpaceBuildKind::Core || !IsOrbitalModuleOperational(site) || dependent->state.owner != owner ||
			dependent->state.kind == SpaceBuildKind::Core || !AreOrbitalModuleFootprintsAdjacent(site.state, dependent->state)) continue;
		// Empty dependents may become inert, while actual stock, held jobs or physical hulls require a surviving completed owned Core.
		if (!HasAlternativeOrbitalCore(dependent->state, site) && HasOrbitalRemovalAccountableItem(*dependent)) return OrbitalRemovalBlock::Dependent;
	}
	AutoRestoreBackup actor(_current_company, owner);
	if (!PlanSpaceStructureRetirement(site)) return OrbitalRemovalBlock::History;
	if (RetireOrbitalModuleUtility(site, false) != UtilityPowerError::None) return OrbitalRemovalBlock::Utility;
	return OrbitalRemovalBlock::None;
}
/** @copydoc IsSpaceStructureSiteAvailable */
bool IsSpaceStructureSiteAvailable(const SpaceStructureState &s)
{
	if (!IsSpaceStructureSiteValid(s, Map::SizeX(), Map::SizeY())) return false;
	// Retained cancelled cargo still occupies the physical site until explicitly collected.
	for (const auto *site : SpaceStructure::Iterate()) if (ReservesSite(*site) && SpaceStructureSitesOverlap(s, site->state)) return false;
	return IsSpaceApproachSiteAvailable(s);
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
/** @copydoc CmdRemoveOrbitalModule */
CommandCost CmdRemoveOrbitalModule(DoCommandFlags flags, SpaceStructureID id, uint64_t generation)
{
	auto *site = ResolveSpaceStructure({id, generation});
	// Exact ownership/incarnation and whole physical admission precede every ledger, receipt and pool mutation.
	if (site == nullptr || ClassifyOrbitalModuleRemoval(*site, _current_company) != OrbitalRemovalBlock::None) return CMD_ERROR;
	const auto history = PlanSpaceStructureRetirement(*site); assert(history.has_value());
	if (!flags.Test(DoCommandFlag::Execute)) return CommandCost{};
	// Retirement already preflighted this exact whole account set and sequence; no participant allocation or intervening mutation can split the operation.
	if (RetireOrbitalModuleUtility(*site, true) != UtilityPowerError::None) return CMD_ERROR;
	const auto reference = GetSpaceStructureReference(*site);
	// Release only this removed incarnation's navigation claim; preserve original entries/cursor so a reused numeric ID never inherits its orders.
	for (auto *craft : SpaceVehicle::Iterate()) {
		auto &queue = craft->space_orders;
		// A paused save immediately after removal must already retain the unresolved flight's cargo and lost-target state, without waiting for another controller tick.
		ForgetSpaceTransitionStructure(*craft, reference);
		/** @param target Original typed native target. @return Whether it names the exact removed static incarnation. */
		auto matches = [&](const SpaceOrderTarget &target) {
			return (target.kind == SpaceOrderTargetKind::Structure || target.kind == SpaceOrderTargetKind::Blueprint) &&
				target.id == reference.structure.base() && target.generation == reference.generation;
		};
		if (queue.approach && matches(queue.approach->target)) ReleaseSpaceApproach(*craft);
		// A current lost target stops only its own action; completed and unrelated queues retain their existing receipts and priority.
		if (queue.cursor < queue.entries.size() && matches(queue.entries[queue.cursor].target)) {
			queue.blocked = SpaceOrderBlock::TargetRemoved; queue.started = false; queue.timer_ticks = 0; queue.work_ticks = 0;
		}
	}
	retirement_history = *history;
	delete site; MarkWholeScreenDirty();
	return CommandCost{};
}
/** Transient whole-transfer admission; quantities/references are views, never a second packet owner or saved registry. */
struct OrbitalEvacuationPlan {
	std::array<uint64_t, NUM_CARGO> quantities{};
	std::array<size_t, NUM_CARGO> packets{};
	std::array<SpaceCraftRef, 2> services{};
	size_t service_count = 0;
	bool manufacture = false; ///< Cancel the exact contained request under the same whole-destination admission.
};

/** @param stock Actual sole native container. @return All saved reference fragments, across every routing bucket. */
static size_t OrbitalOwnerPacketCount(const StationCargoList &stock)
{
	size_t count = 0;
	// Transport bounds count fragments rather than quantities; merging at the destination can only reduce the conservative requirement.
	for (const auto &[hop, packets] : *stock.Packets()) { (void)hop; count += packets.size(); }
	return count;
}

/** Preflight one entire physical owner without splitting or changing it.
 * @param plan Transient all-label admission totals. @param cargo Exact physical label. @param stock Sole source container.
 * @return Whether whole transfer is available without reservation/quantity ambiguity or overflow.
 */
static bool AppendOrbitalEvacuationCargo(OrbitalEvacuationPlan &plan, CargoType cargo, const StationCargoList &stock)
{
	const uint quantity = stock.TotalCount();
	if (quantity == 0) return true;
	// Reserved/native loading slices and unavailable labels cannot silently become generic evacuation inventory.
	if (!IsValidCargoType(cargo) || stock.ReservedCount() != 0 || stock.AvailableCount() != quantity ||
		quantity > UINT64_MAX - plan.quantities[cargo] || !HubCargoTransfer::CanMove(stock, quantity)) return false;
	plan.quantities[cargo] += quantity; plan.packets[cargo] += OrbitalOwnerPacketCount(stock);
	return true;
}

/** Admit the whole exact packet/job operation before any label leaves its original native owner.
 * @param source Native source, allowed to be inert or outage-blocked so its real inventory can be rescued.
 * @param destination Explicit operational adjacent owned inventory that remains valid after source removal.
 * @return Complete transient admission, or absence for any unsupported owner, full label, unsafe dependency or empty action.
 */
static std::optional<OrbitalEvacuationPlan> PlanOrbitalEvacuation(const SpaceStructure &source, const SpaceStructure &destination)
{
	// Transfers follow an actual owned physical edge and existing inventory; proximity, an inert destination or company-ID reuse grants no capacity.
	if (&source == &destination || !Company::IsValidID(_current_company) || source.state.owner != _current_company || destination.state.owner != _current_company ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry ||
		!IsOrbitalModuleOperational(destination) || GetOrbitalModuleStockLimit(destination.state.kind) == 0 ||
		!AreOrbitalModuleFootprintsAdjacent(source.state, destination.state)) return std::nullopt;
	// Moving a last Core's inventory into a module that needs that same Core would strand it as soon as the player removes the source.
	if (source.state.kind == SpaceBuildKind::Core && IsOrbitalModuleOperational(source) && destination.state.kind != SpaceBuildKind::Core &&
		!HasAlternativeOrbitalCore(destination.state, source)) return std::nullopt;
	const auto *definition = GetSpaceConstructionDefinition(source.state.kind);
	if (definition == nullptr || definition->inputs.size() != source.job.inputs.size()) return std::nullopt;
	OrbitalEvacuationPlan plan;
	// Gather every construction label independently of completion; only real unconsumed packets can leave a retained or cancelled job.
	for (uint i = 0; i < source.job.inputs.size(); ++i) {
		const auto cargo = GetCargoTypeByLabel(definition->inputs[i].label);
		// Resolve even empty recipe slots now, so execution can never index a destination inventory using an unavailable content label.
		if (!IsValidCargoType(cargo) || !AppendOrbitalEvacuationCargo(plan, cargo, source.job.inputs[i])) return std::nullopt;
	}
	if (source.module) {
		if (source.module->processing) {
			// Evacuation explicitly cancels unfinished processing under the same no-refund/history admission before any original stock can move.
			if (CmdControlOrbitalRecipe({}, source.index, source.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Cancel)).Failed()) return std::nullopt;
		}
		// Source stock moves first during execution, making room for public service cancellation to return its actual whole parts safely.
		for (CargoType cargo : EnumRange(NUM_CARGO)) if (!AppendOrbitalEvacuationCargo(plan, cargo, source.module->stock[cargo])) return std::nullopt;
		if (source.module->manufacture) {
			// Manufacturing evacuation uses its actual public cancellation path and includes all four declared labels in this same whole-destination preflight.
			if (CmdCancelOrbitalManufacture({}, source.index, source.generation, destination.index, destination.generation).Failed()) return std::nullopt;
			plan.manufacture = true;
			const auto &job = *source.module->manufacture; const auto &recipe = *GetSpaceVehicleConstruction(job.model);
			for (size_t input = 0; input < recipe.inputs.size(); ++input) if (!AppendOrbitalEvacuationCargo(plan, GetCargoTypeByLabel(recipe.inputs[input].label), job.inputs[input])) return std::nullopt;
		}
		const auto jobs = source.module->GetServiceJobs();
		// Every held craft remains physically accounted by the same native berth. Cancelling its job moves no hull, personal hold or manifest allowance.
		for (size_t berth = 0; berth < jobs.size(); ++berth) {
			const auto &job = *jobs[berth];
			if (job.remaining_ticks == 0) {
				// Empty jobs cannot hide unattached spare packets outside the normal stock and cancellation owners.
				if (job.inputs[0].TotalCount() != 0 || job.inputs[1].TotalCount() != 0) return std::nullopt;
				continue;
			}
			// Reproduce public cancellation's role/reference admission before stock is emptied; its later destination is guaranteed empty and bounded.
			if (source.state.phase != SpaceStructurePhase::FrameComplete || source.module->activation_ticks != 0 || berth >= GetOrbitalModuleServiceCapacity(source.state.kind) ||
				job.craft.vehicle == VehicleID::Invalid() || job.craft.generation == 0 || (plan.service_count != 0 && plan.services[0] == job.craft)) return std::nullopt;
			plan.services[plan.service_count++] = job.craft;
			for (uint i = 0; i < job.inputs.size(); ++i) {
				// Native service cancellation returns exactly one original part per label, including a lost craft's still-retained reservation.
				if (job.inputs[i].TotalCount() != 1 || !AppendOrbitalEvacuationCargo(plan, GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1)), job.inputs[i])) return std::nullopt;
			}
		}
	}
	bool useful = plan.manufacture; // A paid packet-free queued order is still an actual cancellable owner.
	if (source.module && source.module->processing) {
		const auto &processing = *source.module->processing;
		// Even packet-free pending work is a useful explicit cancellation; its actual spent energy stays in the source module's historical ledger.
		useful |= processing.enabled || processing.unpowered_work_ticks != 0 || (processing.electrical && processing.electrical->Capture().paid_j != 0);
	}
	// Whole destination preflight includes construction material, existing stock and all cancelled-job parts together, never one label at a time.
	for (CargoType cargo : EnumRange(NUM_CARGO)) {
		if (plan.quantities[cargo] == 0) continue;
		useful = true; const auto &stock = destination.module->stock[cargo];
		const uint64_t capacity = GetOrbitalModuleCargoCapacity(destination, cargo); const auto packets = OrbitalOwnerPacketCount(stock);
		// Conservative fragment and quantity admission prevents a full later label from causing a partial earlier evacuation.
		if (stock.ReservedCount() != 0 || stock.TotalCount() > capacity || plan.quantities[cargo] > capacity - stock.TotalCount() ||
			packets > 64 || plan.packets[cargo] > 64 - packets) return std::nullopt;
	}
	if (!useful) return std::nullopt;
	return plan;
}

/** Move an already admitted complete station container without creating a split packet, payment or owner registry.
 * @param source Sole actual source container. @param destination Preflighted actual destination container.
 */
static void MoveOrbitalEvacuationCargo(StationCargoList &source, StationCargoList &destination)
{
	const uint quantity = source.TotalCount();
	if (quantity == 0) return;
	const bool moved = HubCargoTransfer::Move(source, destination, quantity); assert(moved); (void)moved;
}

/** @copydoc CmdEvacuateOrbitalModule */
CommandCost CmdEvacuateOrbitalModule(DoCommandFlags flags, SpaceStructureID id, uint64_t generation, SpaceStructureID destination_id, uint64_t destination_generation)
{
	auto *source = ResolveSpaceStructure({id, generation}); auto *destination = ResolveSpaceStructure({destination_id, destination_generation});
	if (source == nullptr || destination == nullptr) return CMD_ERROR;
	const auto plan = PlanOrbitalEvacuation(*source, *destination);
	if (!plan) return CMD_ERROR;
	// Mixed query/execute is still whole-operation estimation; no cancellation, packet movement or paid-history change may escape the preflight.
	if (!flags.Test(DoCommandFlag::Execute) || flags.Test(DoCommandFlag::QueryCost)) return CommandCost{};
	if (source->module) {
		if (source->module->processing) {
			const auto cancelled = CmdControlOrbitalRecipe({DoCommandFlag::Execute}, source->index, source->generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Cancel));
			assert(cancelled.Succeeded()); (void)cancelled;
		}
		// Empty original general stock first; held service parts still belong to their exact jobs until their public cancellation below.
		for (CargoType cargo : EnumRange(NUM_CARGO)) MoveOrbitalEvacuationCargo(source->module->stock[cargo], destination->module->stock[cargo]);
	}
	if (source->state.phase == SpaceStructurePhase::Blueprint) {
		const auto cancelled = CmdCancelSpaceBlueprint({DoCommandFlag::Execute}, source->index, source->generation); assert(cancelled.Succeeded()); (void)cancelled;
	}
	const auto &definition = *GetSpaceConstructionDefinition(source->state.kind);
	// Move only unconsumed construction inputs; saved partial progress and every already spent source joule remain spent.
	for (uint i = 0; i < source->job.inputs.size(); ++i) MoveOrbitalEvacuationCargo(source->job.inputs[i], destination->module->stock[GetCargoTypeByLabel(definition.inputs[i].label)]);
	if (plan->manufacture) {
		// The preflight included this complete original batch; cancellation moves it directly to the explicit destination without granting Assembly general storage.
		const auto cancelled = CmdCancelOrbitalManufacture({DoCommandFlag::Execute}, source->index, source->generation, destination->index, destination->generation);
		assert(cancelled.Succeeded()); (void)cancelled;
	}
	// Each admitted native job returns its actual parts into now-empty source stock, preserving manifest serial/allowance and physical craft control.
	for (size_t i = 0; i < plan->service_count; ++i) {
		const auto ref = plan->services[i];
		const auto cancelled = CmdServiceOrbitalProbe({DoCommandFlag::Execute}, source->index, source->generation, ref.vehicle, ref.generation, OrbitalServiceControl::Cancel);
		assert(cancelled.Succeeded()); (void)cancelled;
	}
	if (source->module) {
		// Move the returned service parts under the same all-label capacity admission, without consuming or paying for them a second time.
		for (CargoType cargo : EnumRange(NUM_CARGO)) MoveOrbitalEvacuationCargo(source->module->stock[cargo], destination->module->stock[cargo]);
	}
	if (source->state.phase == SpaceStructurePhase::Cancelled) {
		const auto reference = GetSpaceStructureReference(*source);
		// Empty cancelled construction loses its producer immediately; drop only these exact now-invalid claims so paused saving cannot retain a stale reservation.
		for (auto *craft : SpaceVehicle::Iterate()) {
			auto &queue = craft->space_orders;
			if (queue.approach && queue.approach->target == SpaceOrderTarget{SpaceOrderTargetKind::Structure, reference.structure.base(), reference.generation}) ReleaseSpaceApproach(*craft);
			if (queue.cursor >= queue.entries.size()) continue;
			const auto &target = queue.entries[queue.cursor].target;
			// Stop only the cancelled site's current typed action; hull pose, personal cargo and every unrelated queue remain unchanged.
			if ((target.kind == SpaceOrderTargetKind::Structure || target.kind == SpaceOrderTargetKind::Blueprint) && target.id == reference.structure.base() && target.generation == reference.generation) {
				queue.blocked = SpaceOrderBlock::TargetRemoved; queue.started = false; queue.timer_ticks = 0; queue.work_ticks = 0;
			}
		}
	}
	MarkWholeScreenDirty(); return CommandCost{};
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
	// Retain the existing Core-only command's wire contract; the new module command owns later recipe activation.
	if (site == nullptr || site->state.kind != SpaceBuildKind::Core) return CMD_ERROR;
	return CmdActivateOrbitalModule(flags, id, generation);
}

/** @copydoc CmdActivateOrbitalModule */
CommandCost CmdActivateOrbitalModule(DoCommandFlags flags, SpaceStructureID id, uint64_t generation)
{
	auto *site = ResolveSpaceStructure({id, generation});
	// An explicit, once-only activation follows the complete paid recipe; old saves and previews cannot grant it implicitly.
	if (site == nullptr || site->state.owner != _current_company || !Company::IsValidID(_current_company) ||
		!IsSupportedOrbitalModuleKind(site->state.kind) || site->state.phase != SpaceStructurePhase::FrameComplete || site->module != nullptr) return CMD_ERROR;
	const auto &definition = *GetSpaceConstructionDefinition(site->state.kind);
	// Recheck the complete physical receipt before publishing any functionality; activation never consumes construction inputs a second time.
	if (site->job.work_ticks != definition.work_ticks || site->job.work_fraction != 0 || site->job.work_remainder != 0 ||
		site->state.capital_paid != definition.capital || !IsSupportedSpaceConstructionVersion(site->state.content_version)) return CMD_ERROR;
	// Every recipe label must have exactly its completed consumption and no remaining reserved packet owner.
	for (uint i = 0; i < site->job.inputs.size(); ++i) {
		if (site->job.inputs[i].TotalCount() != 0 || site->job.consumed[i] != definition.inputs[i].quantity) return CMD_ERROR;
	}
	// Extensions acquire no useful capacity without an actually commissioned same-company Core sharing an orthogonal edge.
	if (site->state.kind != SpaceBuildKind::Core && GetOrbitalConstructionCore(site->state) == nullptr) return CMD_ERROR;
	const bool execute = flags.Test(DoCommandFlag::Execute);
	std::unique_ptr<OrbitalModule> prepared;
	// Allocate the complete native functionality before commissioning a shared source, keeping allocation failure free of partial panel registration.
	if (execute) {
		prepared = std::make_unique<OrbitalModule>();
		if (IsOrbitalRecipeKind(site->state.kind)) {
			prepared->processing = CreateEmptyOrbitalRecipeState(*site);
			if (!prepared->processing) return CMD_ERROR;
		}
	}
	UtilityProducerID producer{UINT32_MAX};
	// Only the paid Core/Power recipes contain a panel; Dock activation supplies berths and buffers without creating an energy source.
	if (site->state.kind == SpaceBuildKind::Core || site->state.kind == SpaceBuildKind::Power) {
		const auto source = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::OrbitalPanel, execute);
		if (!source) return CMD_ERROR;
		producer = *source;
	}
	if (execute) {
		prepared->producer = producer;
		prepared->activation_ticks = GetMultiLayerSpaceBudget().core_activation_ticks;
		site->module = std::move(prepared);
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
		(site->state.kind != SpaceBuildKind::Dock && craft->position != GetSpaceStructureApproach(site->state)) || craft->vehstatus.Test(VehState::Crashed) ||
		(craft->manifest && craft->manifest->service_ticks != 0) || quantity == 0 || !IsValidCargoType(cargo) ||
		(craft->cargo.TotalCount() != 0 && craft->cargo_type != cargo)) return CMD_ERROR;
	const auto model = GetSpaceVehicleModel(*craft);
	// Heavy mineral returns and general haulers require their actual shared berth even at a Core/processor. Existing constructor handling retains its geometry contract; a nearby craft cannot borrow another hull's admitted slot.
	const auto berth = site->state.kind == SpaceBuildKind::Dock || model == SPACE_HEAVY_MINER_MODEL || model == 17 ? GetSpaceStructureBerth(*craft, *site) : std::optional<uint8_t>{0};
	// A Dock's two precise admitted slots handle independently; a service job reserves only its own craft and physical berth.
	if (!berth || site->module->GetServiceJobs()[*berth]->remaining_ticks != 0 || HasOrbitalServiceAssignment(*craft)) return CMD_ERROR;
	auto &stock = site->module->stock[cargo];
	const auto stock_capacity = GetOrbitalModuleCargoCapacity(*site, cargo);
	// A valid berth grants handling geometry, not another module function. Assembly has no general stock, and specialized processors exchange only their declared input/product labels.
	if (stock_capacity == 0) return CMD_ERROR;
	const auto capacity = GetSpaceFreightCapacity(model, cargo);
	// Each positive heavy orbital unload is one completed return batch. Pending descents already reserve their own quota; direct handling cannot spend it again or renew an exhausted allowance.
	if (model == SPACE_HEAVY_MINER_MODEL && (!craft->manifest || craft->manifest->returns_remaining <= craft->manifest->pending_returns || craft->mission.phase != SpaceMissionPhase::Holding || capacity == 0)) return CMD_ERROR;
	if (load) {
		const auto label = CargoSpec::Get(cargo)->label;
		// Constructors retain their two material labels; canonical general-freight haulers may load their own positive mass/refit allowance. Mining probes remain extraction-only, and a hauler still approaching or servicing cannot start a competing load.
		const bool constructor = model == 20 && (label == CargoLabel{"STEL"} || label == CargoLabel{"GOOD"});
		const bool hauler = model == 17 && capacity != 0 && craft->mission.phase == SpaceMissionPhase::Holding;
		if (!constructor && !hauler) return CMD_ERROR;
		// Probe freight uses its own mass-bounded hold, separate from paid service/pod compartments.
		if (craft->cargo.TotalCount() + quantity > capacity || !HubCargoTransfer::CanMove(stock, quantity)) return CMD_ERROR;
	} else {
		// Whole destination admission preserves the freight hold when module stock is full or packets cannot move.
		if (stock.TotalCount() + quantity > stock_capacity || !HubCargoTransfer::CanMove(craft->cargo, quantity)) return CMD_ERROR;
		size_t packets = craft->cargo.Packets()->size();
		// Conservatively preflight transport record capacity before a move; packet merging may use fewer slots, never more.
		for (const auto &[hop, list] : *stock.Packets()) { (void)hop; packets += list.size(); }
		if (packets > 64) return CMD_ERROR;
	}
	// A native processing quote must retain a representable revision after any freight movement into or out of its actual packet owner.
	if (site->module->processing && site->module->processing->revision >= UINT64_MAX - 1) return CMD_ERROR;
	// A mixed Execute/QueryCost call remains an estimate; only the real physical transaction may move packets and spend a heavy return allowance.
	if (flags.Test(DoCommandFlag::Execute) && !flags.Test(DoCommandFlag::QueryCost)) {
		const bool moved = load ? HubCargoTransfer::Move(stock, craft->cargo, quantity, craft->tile) : HubCargoTransfer::Move(craft->cargo, stock, quantity, craft->tile);
		assert(moved);
		if (!moved) return CMD_ERROR;
		if (site->module->processing) ++site->module->processing->revision;
		// Quota changes only after its actual positive packet move succeeds; quotes, full receivers, stale targets and failed allocation retain the original allowance and freight.
		if (model == SPACE_HEAVY_MINER_MODEL) --craft->manifest->returns_remaining;
		craft->cargo_type = cargo; craft->cargo_cap = capacity; craft->MarkDirty(); MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc ChangeSpaceStructureOwnership */
void ChangeSpaceStructureOwnership(Owner old_owner, Owner new_owner)
{
	// Takeovers transfer the actual site; company deletion retains inventory under OWNER_NONE before ID reuse.
	for (auto *site : SpaceStructure::Iterate()) {
		if (site->state.owner != old_owner) continue;
		site->state.owner = Company::IsValidID(new_owner) ? new_owner : OWNER_NONE;
		// Ownership changes invalidate earlier processing quotes; an exhausted revision already refuses production and must never wrap into a fresh owner stamp.
		if (site->module && site->module->processing && site->module->processing->revision < UINT64_MAX - 1) ++site->module->processing->revision;
	}
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
			r.module->manufacture_serial = site->module->manufacture_serial;
			if (site->module->manufacture) {
				const auto &job = *site->module->manufacture; auto &copy = r.module->manufacture.emplace();
				static_cast<OrbitalManufactureState &>(copy) = job;
				// Capture every heavy/common label's exact packet IDs and routing, never aggregate replacement stock.
				for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) {
					copy.quantities[input] = static_cast<uint16_t>(job.inputs[input].TotalCount());
					for (const auto &[hop, packets] : *job.inputs[input].Packets()) for (const auto *packet : packets) copy.inputs[input].push_back({packet->index, hop});
				}
			}
			// Both contained service jobs retain their own actual packet owners; the second is canonically empty outside a Dock.
			const auto native_jobs = site->module->GetServiceJobs(); const auto copied_jobs = r.module->GetServiceJobs();
			for (size_t berth = 0; berth < native_jobs.size(); ++berth) {
				static_cast<OrbitalServiceState &>(*copied_jobs[berth]) = *native_jobs[berth];
				for (uint i = 0; i < 2; ++i) {
					copied_jobs[berth]->quantities[i] = static_cast<uint16_t>(native_jobs[berth]->inputs[i].TotalCount());
					for (const auto &[hop, packets] : *native_jobs[berth]->inputs[i].Packets()) for (const auto *packet : packets) copied_jobs[berth]->inputs[i].push_back({packet->index, hop});
				}
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
bool ValidateSpaceStructureRecords(std::span<const SpaceStructureRecord> rows, uint64_t next, uint32_t width, uint32_t height, uint64_t native_tick, SpaceStructureRetirementHistory history)
{
	if (!ValidateOrbitalManufacturingRecords(rows)) return false;
	// Bounded issuance and record count prevent identity reuse and unbounded copied transport.
	if (next == 0 || rows.size() > SpaceStructureID::End().base()) return false;
	// Live and retired incarnations cannot exceed native issuance; subtract only after admitting the retired count to avoid overflow.
	if (history.structures_removed > next - 1 || rows.size() > next - 1 - history.structures_removed ||
		(history.structures_removed == 0 && history.services_completed != 0)) return false;
	std::array<uint64_t, 2> maximum{};
	// Only canonical static recipes bound spent receipts; carried vehicles cannot inflate a retired module's material allowance.
	for (const auto &definition : GetSpaceConstructionDefinitions()) {
		if (definition.width == 0) continue;
		// Every static module must retain the canonical two common-material receipts before it can bound historical consumption.
		if (definition.height == 0 || definition.inputs.size() != maximum.size()) return false;
		for (uint i = 0; i < maximum.size(); ++i) maximum[i] = std::max<uint64_t>(maximum[i], definition.inputs[i].quantity);
	}
	// Use division rather than a potentially overflowing removed-count/product; zeros remain canonical for every older schema.
	for (uint i = 0; i < maximum.size(); ++i) {
		if (maximum[i] == 0) return false;
		const uint64_t needed = history.consumed[i] / maximum[i] + (history.consumed[i] % maximum[i] != 0);
		if (needed > history.structures_removed) return false;
	}
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
			!IsSpaceStructureSiteValid(s, width, height) || !IsSupportedSpaceConstructionVersion(s.content_version) || s.capital_paid != d->capital ||
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
			// Only delivered module roles may activate after full consumption; pending activation cannot contain usable imported stock.
			if (!complete || !IsSupportedOrbitalModuleKind(s.kind) || m.activation_ticks > GetMultiLayerSpaceBudget().core_activation_ticks || m.age_ticks >= Ticks::CARGO_AGING_TICKS) return false;
			// Storage owns physical packets only; its existence cannot mint a service berth, construction slot or electrical source.
			if (s.kind == SpaceBuildKind::Storage && (m.producer != UtilityProducerID{UINT32_MAX} || m.consumer != UtilityConsumerID{UINT32_MAX} ||
				m.assembly_consumer != UtilityConsumerID{UINT32_MAX} || m.services_completed != 0 || !m.power_enabled)) return false;
			// A Power module's paid original panel is mandatory; legacy inventory-only commissioning is confined to historical Cores.
			if (s.kind == SpaceBuildKind::Power && (m.producer == UtilityProducerID{UINT32_MAX} || m.consumer != UtilityConsumerID{UINT32_MAX} ||
				m.assembly_consumer != UtilityConsumerID{UINT32_MAX} || m.services_completed != 0)) return false;
			const bool external_work = s.kind == SpaceBuildKind::Dock || s.kind == SpaceBuildKind::Assembly;
			if (external_work) {
				// Dock service and Assembly work own no panel or Core construction slot; their bounded consumer needs an explicit original native source.
				if (m.producer != UtilityProducerID{UINT32_MAX} || m.assembly_consumer != UtilityConsumerID{UINT32_MAX} || !m.power_enabled ||
					((m.consumer == UtilityConsumerID{UINT32_MAX}) != (m.utility_source == SpaceStructureRef{}))) return false;
				// A manufacturing facility cannot acquire completed service receipts merely by sharing Dock's electrical connection transport.
				if (s.kind == SpaceBuildKind::Assembly && m.services_completed != 0) return false;
				if (m.utility_source != SpaceStructureRef{}) {
					// Connections use issued foreign-proof incarnations and cannot appear during unfinished activation or point back to their own Dock.
					if (m.utility_source.structure >= SpaceStructureID::End() || m.utility_source.generation == 0 || m.utility_source.generation >= next ||
						m.utility_source == r.ref || m.activation_ticks != 0) return false;
				}
			} else {
				// Other roles retain their own original panels and may not acquire a hidden external connection from transport or company-ID reuse.
				if (m.utility_source != SpaceStructureRef{}) return false;
				if (m.producer == UtilityProducerID{UINT32_MAX} && (m.consumer != UtilityConsumerID{UINT32_MAX} || m.assembly_consumer != UtilityConsumerID{UINT32_MAX} || m.services_completed != 0 || !m.power_enabled)) return false;
			}
			const auto jobs = m.GetServiceJobs();
			if (m.manufacture) {
				// Reserved vehicle inputs join the same global packet set as construction, service and stock; no copied job may promise another owner's packet.
				for (const auto &slot : m.manufacture->inputs) for (const auto &[packet, hop] : slot) {
					if (packet >= CargoPacketID::End() || !packets.insert(packet).second || (hop != StationID::Invalid() && hop >= StationID::End())) return false;
				}
			}
			// Validate every contained physical owner, including the mandatory empty second record for historical/non-Dock modules.
			for (size_t berth = 0; berth < jobs.size(); ++berth) {
				const auto &job = *jobs[berth]; const bool active = job.remaining_ticks != 0;
				const auto tuning = GetOrbitalServiceJobTuning(job);
				if (active) {
					// Exact manifest serials and one global craft owner prevent another berth or reused hull claiming the same reserved parts.
					if (!tuning || berth >= GetOrbitalModuleServiceCapacity(s.kind) || m.activation_ticks != 0 || m.consumer == UtilityConsumerID{UINT32_MAX} || job.remaining_ticks > tuning->ticks ||
						job.craft.vehicle == VehicleID::Invalid() || job.craft.generation == 0 || job.services_at_start == UINT64_MAX || m.services_completed == UINT64_MAX ||
						!service_craft.insert({job.craft.vehicle, job.craft.generation}).second) return false;
				} else {
					// Completed/cancelled/inapplicable berths retain no replayable job receipt, hidden occupant or unconsumed material promise.
					if (job.craft != SpaceCraftRef{} || job.paused || job.last_work_interval != 0 || job.services_at_start != 0 || job.model != UINT16_MAX) return false;
				}
				// Each actual job retains its complete immutable bill until final completion. One two-unit native packet or two one-unit packets may finance heavy work; reference counts alone cannot impersonate quantities.
				for (uint i = 0; i < 2; ++i) {
					const auto quantity = active ? tuning->inputs[i] : 0;
					if (job.quantities[i] != quantity || (quantity == 0 ? !job.inputs[i].empty() : job.inputs[i].empty() || job.inputs[i].size() > quantity)) return false;
					for (const auto &[packet, hop] : job.inputs[i]) if (packet >= CargoPacketID::End() || !packets.insert(packet).second || (hop != StationID::Invalid() && hop >= StationID::End())) return false;
				}
			}
			// Each declared physical cargo stays bounded; packet identities cannot alias a job or another facility slot.
			for (CargoType cargo : EnumRange(NUM_CARGO)) {
				// Stock and packet-slot limits bound both gameplay capacity and saved transport before any facility can operate.
				if (m.quantities[cargo] > GetOrbitalModuleStockLimit(s.kind) || m.stock[cargo].size() > 64 || m.stock[cargo].size() > m.quantities[cargo] ||
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
			if (r.module->manufacture) {
				const auto &job = *r.module->manufacture;
				// Every declared recipe quantity must bind original actual packet objects before the native contained job can install.
				for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) {
					uint64_t count = 0;
					for (const auto &[id, hop] : job.inputs[input]) { (void)hop; const auto *packet = CargoPacket::GetIfValid(id); if (packet == nullptr || !owned.insert(id).second) return false; count += packet->Count(); }
					if (count != job.quantities[input]) return false;
				}
			}
			// Bind the reserved RFMT/ELEC quantities to actual packet objects before any native job can be reconstructed.
			for (const auto *job : r.module->GetServiceJobs()) {
				for (uint i = 0; i < 2; ++i) {
					uint64_t count = 0;
					for (const auto &[id, hop] : job->inputs[i]) { (void)hop; const auto *p = CargoPacket::GetIfValid(id); if (p == nullptr || !owned.insert(id).second) return false; count += p->Count(); }
					if (count != job->quantities[i]) return false;
				}
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
				// A processor retains only its published family buffers; native configured meanings, not arbitrary cargo ordinals, determine which physical stock is admissible.
				if (count != 0 && IsOrbitalRecipeKind(r.state.kind) && count > GetOrbitalRecipeCargoCapacity(r.state.kind, CargoSpec::Get(cargo)->label)) return false;
			}
		}
	}
	return true;
}
/** @copydoc ValidateSpaceStructures */
bool ValidateSpaceStructures()
{
	const auto rows = CaptureSpaceStructures();
	// A demolished world's receipts still belong to the selected Space profile after its last live module has gone.
	return ((rows.empty() && next_generation == 1 && retirement_history == SpaceStructureRetirementHistory{}) || GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry) &&
		ValidateSpaceStructureRecords(rows, next_generation, Map::SizeX(), Map::SizeY(), TimerGameTick::counter, retirement_history) && PhysicalInputs(rows) && ValidateOrbitalServices() && ValidateOrbitalRecipes();
}
/** @copydoc InstallLoadedSpaceStructures */
bool InstallLoadedSpaceStructures(std::span<const SpaceStructureRecord> rows, uint64_t next, SpaceStructureRetirementHistory history)
{
	// Installation cannot overwrite live owners or publish invalid/aliased packet inventories.
	if (SpaceStructure::GetNumItems() != 0 || !ValidateSpaceStructureRecords(rows, next, Map::SizeX(), Map::SizeY(), TimerGameTick::counter, history) || !PhysicalInputs(rows)) return false;
	// Reconstruct each sole native owner at its saved identity, preserving packet history without copying quantities.
	for (const auto &r : rows) {
		auto *site = SpaceStructure::CreateAtIndex(r.ref.structure); site->generation = r.ref.generation; site->state = r.state;
		static_cast<OrbitalConstructionState &>(site->job) = r; site->job.age_ticks = r.age_ticks; site->job.consumed = r.consumed;
		for (uint i = 0; i < 2; ++i) for (const auto &[id, hop] : r.inputs[i]) site->job.inputs[i].Append(CargoPacket::Get(id), hop);
		if (r.module) {
			site->module = std::make_unique<OrbitalModule>(); site->module->activation_ticks = r.module->activation_ticks; site->module->age_ticks = r.module->age_ticks;
			static_cast<OrbitalFacilityState &>(*site->module) = *r.module;
			site->module->manufacture_serial = r.module->manufacture_serial;
			if (r.module->manufacture) {
				const auto &copy = *r.module->manufacture;
				site->module->manufacture = std::make_unique<OrbitalManufactureJob>(); auto &job = site->module->manufacture;
				static_cast<OrbitalManufactureState &>(*job) = copy;
				// Reconstruct sole custody from actual loaded packet identities; save loading never moves source stock or requests an allocation.
				for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) for (const auto &[id, hop] : copy.inputs[input]) job->inputs[input].Append(CargoPacket::Get(id), hop);
			}
			// Rebuild each contained berth from its own packet references; old/non-Dock second jobs remain empty rather than receiving parts.
			const auto native_jobs = site->module->GetServiceJobs(); const auto copied_jobs = r.module->GetServiceJobs();
			for (size_t berth = 0; berth < native_jobs.size(); ++berth) {
				static_cast<OrbitalServiceState &>(*native_jobs[berth]) = *copied_jobs[berth];
				for (uint i = 0; i < 2; ++i) for (const auto &[id, hop] : copied_jobs[berth]->inputs[i]) native_jobs[berth]->inputs[i].Append(CargoPacket::Get(id), hop);
			}
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
		if (r.module) {
			const auto actual = it->module->GetServiceJobs(), expected = r.module->GetServiceJobs();
			// Rebuilding the new contained manufacturing owner must preserve the exact per-label ordering as well as the whole quantity.
			if (r.module->manufacture && (!it->module->manufacture || it->module->manufacture->inputs != r.module->manufacture->inputs)) return false;
			// Whole installation compares both actual service owners, preventing a second berth's packet order from disappearing on reload.
			for (size_t berth = 0; berth < actual.size(); ++berth) if (actual[berth]->inputs != expected[berth]->inputs) return false;
		}
	}
	next_generation = next; retirement_history = history; return true;
}
/** @copydoc ComputeSpaceStructureDigest */
uint64_t ComputeSpaceStructureDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	/** @param value Canonical integer whose big-endian bytes enter the native digest. */
	auto add = [&](uint64_t value) { for (int shift = 56; shift >= 0; shift -= 8) { digest ^= static_cast<uint8_t>(value >> shift); digest *= 1099511628211ULL; } };
	add(next_generation);
	add(ComputeOrbitalRecipeDigest()); // Private progress, recipe/material/residue and retired processing history remain part of the same native authority digest.
	// Historical spent receipts remain part of the same native digest after pool identities become reusable.
	add(retirement_history.structures_removed); add(retirement_history.consumed[0]); add(retirement_history.consumed[1]); add(retirement_history.services_completed);
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
			add(r.module->manufacture_serial); add(r.module->manufacture.has_value());
			if (r.module->manufacture) {
				const auto &job = *r.module->manufacture;
				add(to_underlying(job.phase)); add(job.model); add(job.serial); add(job.source.structure.base()); add(job.source.generation);
				add(job.capital_paid); add(job.berth); add(job.reservation_interval);
				// Saved paid work, exact completion material receipt and original product identity must survive every digest/reload boundary.
				add(job.work_ticks); add(job.last_work_interval); add(job.work_energy_j); add(job.product.vehicle.base()); add(job.product.generation);
				for (const auto quantity : job.consumed) add(quantity);
				add(job.clone.has_value());
				if (job.clone) {
					const auto &clone = *job.clone;
					add(clone.source.vehicle.base()); add(clone.source.generation); add(clone.name.size());
					// Every captured name byte and typed instruction remains immutable request intent; source changes cannot silently rewrite a pending paid clone during save/retry.
					for (const auto byte : clone.name) add(static_cast<uint8_t>(byte));
					add(clone.orders.size());
					for (const auto &order : clone.orders) { add(to_underlying(order.action)); add(to_underlying(order.target.kind)); add(order.target.id); add(order.target.generation); }
				}
				add(job.replacement.has_value());
				if (job.replacement) {
					const auto &intent = *job.replacement;
					add(intent.source.vehicle.base()); add(intent.source.generation); add(intent.source_model); add(intent.resume); add(intent.settled); add(intent.sale_credit);
				}
				// Every common/heavy input retains its full packet provenance in the same native structure digest through reservation and cancellation.
				for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) {
					add(job.quantities[input]); add(job.inputs[input].size());
					for (const auto &[id, hop] : job.inputs[input]) {
						const auto *packet = CargoPacket::Get(id); add(id.base()); add(hop.base()); add(packet->Count()); add(packet->GetPeriodsInTransit());
						add(static_cast<int64_t>(packet->GetFeederShare())); add(packet->GetSourceTile().base()); add(packet->GetFirstStation().base()); add(to_underlying(packet->GetSource().type)); add(packet->GetSource().id);
					}
				}
			}
			add(r.module->producer.base()); add(r.module->consumer.base()); add(r.module->assembly_consumer.base()); add(r.module->power_enabled); add(r.module->services_completed);
			add(r.module->utility_source.structure.base()); add(r.module->utility_source.generation);
			// Each berth's work receipt and packet provenance joins the digest independently of shared freight inventory.
			for (const auto *job : r.module->GetServiceJobs()) {
				add(job->craft.vehicle.base()); add(job->craft.generation); add(job->remaining_ticks); add(job->paused); add(job->last_work_interval); add(job->services_at_start); add(job->model);
				for (uint i = 0; i < 2; ++i) {
					add(job->quantities[i]); add(job->inputs[i].size());
					for (const auto &[id, hop] : job->inputs[i]) {
						const auto *p = CargoPacket::Get(id); add(id.base()); add(hop.base()); add(p->Count()); add(p->GetPeriodsInTransit()); add(static_cast<int64_t>(p->GetFeederShare()));
						add(p->GetSourceTile().base()); add(p->GetFirstStation().base()); add(to_underlying(p->GetSource().type)); add(p->GetSource().id);
					}
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
