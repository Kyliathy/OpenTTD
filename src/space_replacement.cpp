/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_replacement.cpp Prepared native replacement rights, physical handover and conserved source holdings.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
 * Pure preparation never creates a hull, moves a packet, spends work or credits a projected sale.
 */
#include "stdafx.h"
#include "space_replacement.h"
#include "space_vehicle.h"
#include "space_clone.h"
#include "space_purchase.h"
#include "space_manifest.h"
#include "space_transition.h"
#include "space_structure.h"
#include "space_approach.h"
#include "space_structure_cmd.h"
#include "orbital_manufacturing.h"
#include "command_func.h"
#include "orbital_service.h"
#include "launch_operations.h"
#include "company_func.h"
#include "company_base.h"
#include "multilayer_content.h"
#include "vehicle_cmd.h"
#include "autoreplace_cmd.h"
#include "vehicle_func.h"
#include "gfx_func.h"
#include "group_cmd.h"
#include "asteroid.h"
#include "ai/ai.hpp"
#include "script/api/script_event_types.hpp"
#include "station_base.h"
#include "engine_base.h"
#include "group.h"
#include "map_func.h"
#include "table/strings.h"
#include "safeguards.h"
#include <set>

/** @param source Original native index followed by an existing viewport. @param product Actual new index to follow after prepared handover. */
extern void ChangeVehicleViewports(VehicleID source, VehicleID product);
/** @param source Original native window identity. @param product Actual new native window identity, preserving its open configuration. */
extern void ChangeVehicleViewWindow(VehicleID source, VehicleID product);
/** @param source Original actual news reference. @param product Actual paid replacement reference for that existing news item. */
extern void ChangeVehicleNews(VehicleID source, VehicleID product);

static const SpaceVehicle *retiring_source = nullptr; ///< Private synchronous native sell permission; contained producer intent remains the only saved replacement authority.

/** @copydoc IsSpaceReplacementRetirement */
bool IsSpaceReplacementRetirement(const SpaceVehicle &source) { return retiring_source == &source; }

/** @copydoc QueueSpaceReplacementOnArrival */
void QueueSpaceReplacementOnArrival(SpaceVehicle &source)
{
	AutoRestoreBackup actor(_current_company, source.owner);
	// A completed physical arrival, actual native policy and whole producer quote are all required. Idle polling would silently recreate a deliberately cancelled request every tick.
	if (source.construction.placement != SpacePlacement::Orbital || source.vehstatus.Test(VehState::Stopped) || CmdAutoreplaceVehicle({}, source.index).Failed()) return;
	VehicleEnteredDepotThisTick(&source);
}

/** @copydoc SetPendingSpaceReplacementResume */
void SetPendingSpaceReplacementResume(SpaceCraftRef source, bool resume)
{
	const auto *original = ResolveSpaceCraft(source);
	if (original == nullptr) return;
	// Running intent lives with the one actual unfinished orbital job; completed receipts and Earth dormant bodies cannot inherit later source commands.
	for (auto *site : SpaceStructure::Iterate()) {
		if (!site->module || !site->module->manufacture || site->state.owner != original->owner) continue;
		auto &intent = site->module->manufacture->replacement;
		if (intent && !intent->settled && intent->source == source) intent->resume = resume;
	}
}

/** @copydoc RestoreCancelledSpaceReplacement */
void RestoreCancelledSpaceReplacement(const SpaceReplacementIntent &intent, uint16_t target_model, CompanyID owner, const SpaceReplacementSite &site)
{
	// Cancellation never revives a lost/foreign source or starts active/unsafe work. Current whole holdings and exact berth must still satisfy the original handover contract.
	if (!intent.resume || !PrepareSpaceReplacement(intent, target_model, owner, site)) return;
	auto *source = ResolveSpaceCraft(intent.source); assert(source != nullptr);
	source->vehstatus.Reset(VehState::Stopped); source->MarkDirty();
}

/** @copydoc CmdCancelEarthSpaceReplacement */
CommandCost CmdCancelEarthSpaceReplacement(DoCommandFlags flags, VehicleID id, uint64_t generation)
{
	auto *product = ResolveSpaceCraft({id, generation});
	// The actual paid unfinished Earth body alone owns this request, including legitimate unhoming; settled receipts cannot become a second cancellation/refund path.
	if (product == nullptr || product->owner != _current_company || !Company::IsValidID(_current_company) || !product->construction.replacement ||
		product->construction.replacement->settled || product->construction.remaining_ticks == 0 || product->construction.assembly != SpaceStructureRef{}) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute) && !flags.Test(DoCommandFlag::QueryCost)) {
		product->construction.replacement.reset(); product->MarkDirty(); MarkWholeScreenDirty();
	}
	return CommandCost{};
}

/** @copydoc AreSpaceReplacementModelsCompatible */
bool AreSpaceReplacementModelsCompatible(uint16_t source_model, uint16_t target_model)
{
	const bool source = CanCommissionSpaceVehicle(source_model, MultiLayerCommissioningFacility::LaunchCenter, true) ||
		CanCommissionSpaceVehicle(source_model, MultiLayerCommissioningFacility::OrbitalAssembly, true);
	const bool target = CanCommissionSpaceVehicle(target_model, MultiLayerCommissioningFacility::LaunchCenter, true) ||
		CanCommissionSpaceVehicle(target_model, MultiLayerCommissioningFacility::OrbitalAssembly, true);
	// Only canonical delivered native roles can replace one another; a rocket's flight history cannot become another role's controller.
	return source && target && ((source_model == 14) == (target_model == 14));
}

/** @copydoc ValidateSpaceReplacementIntent */
bool ValidateSpaceReplacementIntent(const SpaceReplacementIntent &intent, uint16_t target_model, uint64_t next_vehicle, SpaceCraftRef product)
{
	const auto *source = GetSpaceVehicleConstruction(intent.source_model);
	// Only actual canonical vehicle recipes and already issued original incarnations may authorize a paid retirement; module/passive-shell content is not a replacement hull.
	if (source == nullptr || !AreSpaceReplacementModelsCompatible(intent.source_model, target_model) ||
		intent.source.vehicle >= VehicleID::End() || intent.source.generation == 0 || intent.source.generation >= next_vehicle) return false;
	// Pending production owns no sale proceeds. A settled receipt is bounded by the original paid capital, never a second speculative refund.
	if (intent.sale_credit > source->capital || (!intent.settled && intent.sale_credit != 0)) return false;
	if (intent.settled && product == SpaceCraftRef{}) return false; // A completed retirement must name an actual already issued product; an empty completion cannot authorize lost-hull accounting.
	// An issued new Earth or completed orbital hull must postdate the old incarnation, even if later allocation reuses the same numerical pool slot.
	if (product != SpaceCraftRef{} && (product.vehicle >= VehicleID::End() || product.generation >= next_vehicle || product.generation <= intent.source.generation)) return false;
	return true;
}

/** @copydoc ValidateSpaceReplacementLinks */
bool ValidateSpaceReplacementLinks(std::span<const SpaceStructureRecord> structures, std::span<const SpaceProductionRecord> products, uint64_t next_vehicle)
{
	std::set<std::pair<VehicleID, uint64_t>> originals;
	/** Check one actual contained producer without resolving any live world identity.
	 * @param intent Original source and retirement receipt.
	 * @param model Containing paid target recipe.
	 * @param product Actual issued product, or empty for pending orbital work.
	 * @return Whether issuance, unique authority and any surviving immutable source receipt agree.
	 */
	const auto valid = [&](const SpaceReplacementIntent &intent, uint16_t model, SpaceCraftRef product) {
		// One original incarnation can authorize only one paid replacement, including histories retained after completion; numeric slot reuse remains a different incarnation.
		if (!ValidateSpaceReplacementIntent(intent, model, next_vehicle, product) || !originals.emplace(intent.source.vehicle, intent.source.generation).second) return false;
		/** @param row Detached same-file receipt. @return Whether it is the exact original incarnation. */
		const auto source = std::ranges::find_if(products, [&](const auto &row) { return row.craft == intent.source; });
		if (intent.settled) return source == products.end();
		if (source == products.end()) return true; // Legitimate source loss leaves paid pending work blocked; a numeric successor never inherits its assets or authority.
		const auto *recipe = GetSpaceVehicleConstruction(intent.source_model);
		// A surviving original retains its actual immutable paid recipe; mutable ownership, configuration and placement may legitimately change while work waits.
		return recipe != nullptr && source->state.model == intent.source_model && IsSupportedSpaceVehicleContentVersion(source->state.content_version, source->state.model) && source->state.capital_paid == recipe->capital;
	};
	// Orbital authority belongs exclusively to the actual manufacturing job; cloning and replacement cannot both claim its one new product or its configuration.
	for (const auto &row : structures) {
		if (!row.module || !row.module->manufacture) continue;
		const auto &job = *row.module->manufacture;
		if (!job.replacement) continue;
		if (job.clone || job.replacement->settled != (job.phase == OrbitalManufacturePhase::Complete) || !valid(*job.replacement, job.model, job.product)) return false;
	}
	// Earth already owns a real paid Assembling body. It retains this receipt after commissioning, but never acquires a second orbital job or an automatic running request.
	for (const auto &row : products) {
		const auto &state = row.state;
		if (!state.replacement) continue;
		const auto &intent = *state.replacement;
		if (state.assembly != SpaceStructureRef{} || intent.resume || !IsSupportedSpaceVehicleContentVersion(state.content_version, state.model) || !valid(intent, state.model, row.craft)) return false;
		if (intent.settled) {
			if (state.remaining_ticks != 0) return false;
		} else {
			// Center destruction can unhome a real unfinished body; only unfinished assembly owns a pending Earth retirement request, never a completed freely deployed hull.
			if (state.remaining_ticks == 0 || (state.placement != SpacePlacement::Assembling && state.placement != SpacePlacement::Unhomed)) return false;
		}
	}
	return true;
}

/** @copydoc HasPendingSpaceReplacement */
bool HasPendingSpaceReplacement(SpaceCraftRef source)
{
	// The existing physical producer is the sole pending authority. Scan contained jobs, never a parallel identity registry that could survive cancellation or company loss incorrectly.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (!site->module) continue;
		const auto *job = site->module->manufacture.get();
		if (job != nullptr && job->replacement && !job->replacement->settled && job->replacement->source == source) return true;
	}
	// Earth commissioning already has a real Assembling hull; its contained intent locks the same historical original without allocating a proxy or copying source assets.
	for (const auto *product : SpaceVehicle::Iterate()) {
		const auto &intent = product->construction.replacement;
		if (intent && !intent->settled && intent->source == source) return true;
	}
	return false;
}

/** Resolve an actual current handover berth without moving the old hull or inventing a remote depot.
 * @param source Actual surviving paid old hull.
 * @param owner Current physical producer owner.
 * @param site Existing Earth center or orbital factory with its real adjacent inventory berth.
 * @return Whether the source is physically waiting inside that same owned production facility.
 */
static bool HasReplacementHandover(const SpaceVehicle &source, CompanyID owner, const SpaceReplacementSite &site)
{
	if (site.center != LaunchCenterRef{}) {
		const auto *center = ResolveLaunchCenter(site.center);
		if (center == nullptr) return false;
		const auto storage = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Storage);
		const auto pad = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Pad);
		const bool stored = storage && storage->Contains(source.construction.surface_tile);
		const bool on_pad = GetSpaceVehicleModel(source) == 14 && pad && pad->Contains(source.construction.surface_tile);
		// Earth storage/workshop share their actual original native center; another facility or an orbital hull cannot transfer holdings through Surface coordinates.
		return site.factory == SpaceStructureRef{} && site.inventory == SpaceStructureRef{} && center->owner == owner && center->launch_operations != nullptr &&
			source.construction.placement == SpacePlacement::SurfaceStored && source.construction.home == site.center && source.position == source.home &&
			source.vehstatus.Test(VehState::Stopped) && source.vehstatus.Test(VehState::Hidden) && (stored || on_pad);
	}
	const auto *factory = ResolveSpaceStructure(site.factory), *inventory = ResolveSpaceStructure(site.inventory);
	// Orbital handover requires this job's actual adjacent owned inventory and precise retained source berth; a numerical factory receipt alone cannot move cargo across space.
	return factory != nullptr && inventory != nullptr && factory->state.kind == SpaceBuildKind::Assembly && factory->state.owner == owner && inventory->state.owner == owner &&
		(inventory->state.kind == SpaceBuildKind::Core || inventory->state.kind == SpaceBuildKind::Storage) && IsOrbitalModuleOperational(*factory) && IsOrbitalModuleOperational(*inventory) &&
		AreOrbitalModuleFootprintsAdjacent(factory->state, inventory->state) && source.construction.placement == SpacePlacement::Orbital && GetSpaceStructureBerth(source, *inventory).has_value();
}

/** @copydoc PrepareSpaceReplacement */
std::optional<PreparedSpaceReplacement> PrepareSpaceReplacement(const SpaceReplacementIntent &intent, uint16_t target_model, CompanyID owner, const SpaceReplacementSite &site)
{
	// Settlement must re-resolve this exact original incarnation; removal, takeover of only one owner or numerical-ID reuse retains the full paid producer for retry/cancellation.
	if (intent.settled || !ValidateSpaceReplacementIntent(intent, target_model, GetNextSpaceVehicleGeneration())) return std::nullopt;
	const auto *source = ResolveSpaceCraft(intent.source);
	const auto *recipe = GetSpaceVehicleConstruction(intent.source_model);
	// Only the surviving canonical material-paid source with this producer's current owner can settle; legacy demonstration hulls and foreign receipts supply no retirement authority.
	if (source == nullptr || source->owner != owner || !IsSpacePurchaseEngine(source->engine_type) || GetSpaceVehicleModel(*source) != intent.source_model ||
		source->construction.model != intent.source_model || !IsSupportedSpaceVehicleContentVersion(source->construction.content_version, source->construction.model) || source->construction.capital_paid != recipe->capital ||
		source->vehstatus.Test(VehState::Crashed) || !HasReplacementHandover(*source, owner, site)) return std::nullopt;
	const std::array receipt{SpaceProductionRecord{intent.source, source->construction}};
	// Reuse the native detached paid-receipt/placement validator before retiring an old owner; incomplete or forged deployment cannot acquire authority from an otherwise real berth.
	if (!ValidateSpaceProductionRecords(receipt, Map::SizeX(), Map::SizeY())) return std::nullopt;
	// Active navigation/mining/service/loading owners cannot be reset as replacement configuration; their original work and every packet remain with the old hull until genuinely idle.
	if (source->mission.phase != SpaceMissionPhase::Holding || source->space_orders.started || source->space_orders.work_ticks != 0 || source->space_orders.timer_ticks != 0 ||
		HasOrbitalServiceAssignment(*source) || IsSpaceFlightActive(*source) || source->cargo_subtype != 0 || !source->Orders().empty() || source->IsOrderListShared() ||
		source->cargo.TotalCount() != source->cargo.ActionCount(VehicleCargoList::MoveToAction::Keep)) return std::nullopt;
	if (source->value < Money{0} || source->value > Money{recipe->capital}) return std::nullopt; // Native depreciation bounds the one eventual credit; a malformed value cannot create money.
	const auto *group = Group::GetIfValid(source->group_id);
	if (!IsDefaultGroupID(source->group_id) && (group == nullptr || group->owner != owner || group->vehicle_type != VehicleType::Space)) return std::nullopt; // Retain actual native group accounting rights.
	const auto *engine = Engine::GetIfValid(GetSpaceEngine(target_model));
	// Preparation requires the already registered matching target role; neither a missing mapping nor another native Engine can supply speculative destination capacity.
	if (engine == nullptr || engine->type != VehicleType::Space || engine->VehInfo<SpaceVehicleInfo>().model != target_model) return std::nullopt;
	const auto cargo = source->cargo.TotalCount() == 0 ? engine->GetDefaultCargoType() : source->cargo_type;
	const auto capacity = GetSpaceFreightCapacity(target_model, cargo);
	const auto placement = site.center == LaunchCenterRef{} ? SpacePlacement::Orbital : SpacePlacement::SurfaceStored;
	if (!CanRetainSpaceFreight(target_model, placement, cargo, source->cargo.TotalCount())) return std::nullopt; // Share native whole-hold/placement policy; retirement cannot silently discard or relabel an incompatible packet.
	SpaceConstructionState target; target.model = target_model; target.placement = placement; target.content_version = MULTILAYER_SPACE_CONTENT_VERSION;
	// A whole hold can fit by mass while its provenance is unsupported by the new role. Keep every original packet with the source until native custody policy admits the entire handover.
	for (const auto *packet : *source->cargo.Packets()) if (!CanRetainSpaceCargoPacket(target, *packet)) return std::nullopt;
	if (source->manifest) {
		const auto &manifest = *source->manifest;
		// Carried hulls, descent shells and reserved work have additional owners; wait rather than retarget them or publish an over-budget return allowance on the new hull.
		if (manifest.passenger != SpaceCraftRef{} || manifest.carrier != SpaceCraftRef{} || !manifest.pods.empty() || manifest.service_ticks != 0 || manifest.pending_returns != 0 ||
			manifest.returns_remaining > GetMultiLayerSpaceBudget().returns_per_service) return std::nullopt;
		// Transfer a complete existing service manifest only when each actual compartment fits the new immutable role; used return allowance and service/fuel history are never refreshed.
		for (uint8_t supply = 0; supply < SPACE_SUPPLY_COUNT; ++supply) {
			if (manifest.supplies[supply].TotalCount() > GetSpaceSupplyCapacity(target_model, static_cast<SpaceSupply>(supply))) return std::nullopt;
		}
	}
	SpaceCloneIntent configuration{intent.source, source->name, source->space_orders.entries};
	// Reuse the existing bounded role/name/target policy for independent instructions; actual surviving foreign targets cannot acquire rights through a replacement.
	if (!ValidateSpaceCloneIntent(configuration, target_model, GetNextSpaceVehicleGeneration(), GetNextSpaceStructureGeneration()) || !CanApplySpaceCloneIntent(configuration, owner)) return std::nullopt;
	PreparedSpaceReplacement prepared;
	prepared.source = intent.source; prepared.name = std::move(configuration.name); prepared.orders = source->space_orders; prepared.orders.approach.reset();
	prepared.cargo = cargo; prepared.capacity = capacity; prepared.sale_credit = static_cast<uint32_t>(source->value);
	return prepared;
}

/** @copydoc ReplaceSpaceVehicle */
CommandCost ReplaceSpaceVehicle(DoCommandFlags flags, SpaceVehicle &source, EngineID engine)
{
	// Company policy supplies actual existing native Engines, while this physical admission still requires the original paid owned source and supported profile.
	if (!Company::IsValidID(_current_company) || source.owner != _current_company || !IsSpacePurchaseEngine(source.engine_type) || !IsSpacePurchaseEngine(engine) ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return CMD_ERROR;
	const auto original = GetSpaceCraftReference(source);
	if (HasPendingSpaceReplacement(original)) return CommandCost(STR_ERROR_AUTOREPLACE_NOTHING_TO_DO);
	const auto target_model = Engine::Get(engine)->VehInfo<SpaceVehicleInfo>().model;
	SpaceReplacementIntent intent;
	intent.source = original; intent.source_model = GetSpaceVehicleModel(source); intent.resume = !source.vehstatus.Test(VehState::Stopped);
	if (!ValidateSpaceReplacementIntent(intent, target_model, GetNextSpaceVehicleGeneration())) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::QueryCost)) flags.Reset(DoCommandFlag::Execute);
	if (source.construction.placement == SpacePlacement::SurfaceStored) {
		const auto home = source.construction.home; const auto *center = ResolveLaunchCenter(home);
		// Existing Surface storage/workshop ownership selects the physical producer. A historical missing center cannot borrow another visible launch station.
		if (center == nullptr || center->owner != source.owner || !center->launch_center) return CMD_ERROR;
		const SpaceReplacementSite site{{}, {}, home};
		if (!PrepareSpaceReplacement(intent, target_model, source.owner, site)) return CMD_ERROR;
		const auto workshop = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Workshop);
		if (!workshop) return CMD_ERROR;
		auto quote = PurchaseSpaceVehicleAtFacility({}, workshop->tile, engine, INVALID_CARGO);
		if (quote.cost.Failed()) return quote.cost;
		if (!flags.Test(DoCommandFlag::Execute)) return quote.cost;
		// The explicit original-hull context has now passed preflight. Delegate the genuine Earth producer without its deliberately refused context-free AutoReplace flag.
		auto result = PurchaseSpaceVehicleAtFacility(DoCommandFlags{flags}.Reset(DoCommandFlag::AutoReplace), workshop->tile, engine, INVALID_CARGO);
		if (result.cost.Failed()) return result.cost;
		auto *product = SpaceVehicle::Get(result.vehicle); assert(product != &source);
		product->construction.replacement = intent; source.vehstatus.Set(VehState::Stopped); source.MarkDirty();
		return result.cost;
	}
	if (source.construction.placement != SpacePlacement::Orbital) return CMD_ERROR;
	SpaceStructure *inventory = nullptr;
	// An exact current owned inventory berth selects the old hull's physical handover source; neither its birthplace nor nearby camera coordinates can authorise remote asset movement.
	for (auto *candidate : SpaceStructure::Iterate()) {
		if (candidate->state.owner != source.owner || (candidate->state.kind != SpaceBuildKind::Core && candidate->state.kind != SpaceBuildKind::Storage) || !GetSpaceStructureBerth(source, *candidate)) continue;
		if (inventory != nullptr) return CMD_ERROR; // Ambiguous physical ownership must refuse rather than choose a pool-order winner.
		inventory = candidate;
	}
	if (inventory == nullptr) return CMD_ERROR;
	SpaceStructure *factory = nullptr; CommandCost cost{INVALID_STRING_ID};
	// Quote the original typed manufacturing producer for each real adjacent candidate. Only one eligible factory may admit an ordinary replacement that has no explicit destination argument.
	for (auto *candidate : SpaceStructure::Iterate()) {
		auto quote = QueueOrbitalReplacement({}, *candidate, *inventory, target_model, intent);
		if (quote.Failed()) continue;
		if (factory != nullptr) return CMD_ERROR; // Another eligible real producer needs explicit selection, never an implicit second request or substituted inventory.
		factory = candidate; cost = std::move(quote);
	}
	if (factory == nullptr) return cost;
	if (!flags.Test(DoCommandFlag::Execute)) return cost;
	cost = QueueOrbitalReplacement(flags, *factory, *inventory, target_model, intent);
	if (cost.Failed()) return cost;
	assert(factory->module && factory->module->manufacture);
	assert(factory->module->manufacture->replacement && factory->module->manufacture->replacement->source == original);
	source.vehstatus.Set(VehState::Stopped); source.MarkDirty();
	return cost;
}

/** @copydoc CommitSpaceReplacement */
void CommitSpaceReplacement(SpaceReplacementIntent &intent, SpaceVehicle &product, PreparedSpaceReplacement &&prepared)
{
	if (intent.settled) return; // A completed contained receipt cannot retire/refund again, including after the old numerical pool slot has been reused.
	auto *source = ResolveSpaceCraft(prepared.source);
	assert(source != nullptr && source != &product && source->owner == product.owner && intent.source == prepared.source && !intent.settled);
	assert(prepared.orders.approach == std::nullopt && product.cargo.TotalCount() == 0);
	AutoRestoreBackup actor(_current_company, product.owner);
	const auto original = prepared.source, issued = GetSpaceCraftReference(product);
	const auto old_id = source->index; const auto owner = product.owner;
	// Native group membership and unit/profit configuration remain their existing owners; prepared name storage already fits before the old identity can be retired.
	product.name = std::move(prepared.name);
	auto [group_cost, group] = CmdAddVehicleGroup(DoCommandFlag::Execute, source->group_id, product.index, false, VehicleListIdentifier{});
	assert(group_cost.Succeeded() && group == source->group_id);
	product.CopyVehicleConfigAndStatistics(source); GroupStatistics::AddProfitLastYear(&product);
	// Keep the actual new factory approach, while independent instructions/current index preserve the old idle action without transferring a physical lease or partial work.
	auto approach = std::move(product.space_orders.approach);
	product.space_orders = std::move(prepared.orders); product.space_orders.approach = std::move(approach);
	product.cargo_type = prepared.cargo; product.cargo_cap = prepared.capacity;
	const auto quantity = source->cargo.TotalCount();
	const auto moved = source->cargo.Shift(quantity, &product.cargo);
	assert(moved == quantity && source->cargo.TotalCount() == 0);
	// Moving the sole existing manifest/controller retains packet provenance, used allowance, service/fuel history and completed rocket missions; the fresh producer allowance is not added again.
	product.manifest = std::move(source->manifest); product.transition = std::move(source->transition);
	product.extracted_units = source->extracted_units; product.mission.cost_remainder = source->mission.cost_remainder;
	product.vehstatus.Set(VehState::Stopped);
	if (intent.resume && product.construction.placement == SpacePlacement::Orbital) product.vehstatus.Reset(VehState::Stopped); // Only a deployed new hull resumes the recorded running intent; dormant Earth products keep native stopped custody.
	source->vehstatus.Set(VehState::Stopped);
	ChangeVehicleViewports(old_id, product.index); ChangeVehicleViewWindow(old_id, product.index); ChangeVehicleNews(old_id, product.index);
	RetargetSpaceVehicleInspector(original, issued);
	// This exact prepared source alone may retire away from its old home berth, after every holding moved. Invoke the original sell/destruction owner directly, then its native finance primitive once.
	{
		AutoRestoreBackup retiring(retiring_source, static_cast<const SpaceVehicle *>(source));
		const auto sale = CmdSellVehicle({DoCommandFlag::Execute, DoCommandFlag::AutoReplace}, old_id, false, false, ClientID::Invalid);
		assert(sale.Succeeded() && sale.GetCost() == -Money{prepared.sale_credit});
		SubtractMoneyFromCompany(owner, sale);
	}
	intent.sale_credit = prepared.sale_credit; intent.settled = true;
	AI::NewEvent(owner, new ScriptEventVehicleAutoReplaced(old_id, product.index));
	product.MarkDirty();
}
