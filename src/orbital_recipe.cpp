/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_recipe.cpp Native module packet participant for the shared all-input recipe and electrical-work owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C exact paid/residue history remains with the native module.
 * Outputs are fully allocated before any material debit. No station delivery callback, additional stock owner or private electrical balance is introduced.
 */
#include "stdafx.h"
#include "orbital_recipe.h"
#include "space_structure.h"
#include "hub_cargo.h"
#include "company_base.h"
#include "company_func.h"
#include "timer/timer_game_calendar.h"
#include "safeguards.h"

/** @copydoc GetDefaultOrbitalRecipe */
MultiLayerRecipeID GetDefaultOrbitalRecipe(SpaceBuildKind kind)
{
	switch (kind) {
		case SpaceBuildKind::Steelworks: return MultiLayerRecipeID{"OSTL"};
		case SpaceBuildKind::Goodsworks: return MultiLayerRecipeID{"OGDS"};
		case SpaceBuildKind::Refinery: return MultiLayerRecipeID{"OREF"};
		case SpaceBuildKind::IceProcessor: return MultiLayerRecipeID{"PICE"};
		default: return {};
	}
}

/** @copydoc IsOrbitalRecipeKind */
bool IsOrbitalRecipeKind(SpaceBuildKind kind) { return GetDefaultOrbitalRecipe(kind) != MultiLayerRecipeID{}; }

/** @copydoc GetOrbitalRecipeDefinition */
const MultiLayerManufacturingRecipe *GetOrbitalRecipeDefinition(SpaceBuildKind kind, MultiLayerRecipeID choice)
{
	const auto primary = GetDefaultOrbitalRecipe(kind); const MultiLayerManufacturingRecipe *family = nullptr;
	if (primary == MultiLayerRecipeID{}) return nullptr;
	// The existing published primary determines family membership; translated module names or another layer's recipe cannot authorize a local alternative.
	for (const auto &record : GetMultiLayerManufacturingRecipes()) if (record.layer == MapLayer::Space && record.recipe.id == primary) family = &record;
	if (family == nullptr) return nullptr;
	if (choice == MultiLayerRecipeID{}) choice = primary;
	// Resolve the requested stable ID within that exact content-owned family, preserving every numerical input/output/work field in its original definition.
	for (const auto &record : GetMultiLayerManufacturingRecipes()) {
		if (record.layer == MapLayer::Space && record.facility == family->facility && record.recipe.id == choice) return &record;
	}
	return nullptr;
}

/** @copydoc GetOrbitalRecipeCargoCapacity */
uint16_t GetOrbitalRecipeCargoCapacity(SpaceBuildKind kind, CargoLabel label)
{
	const auto *family = GetOrbitalRecipeDefinition(kind); uint16_t capacity = 0;
	if (family == nullptr) return 0;
	// Retain the bounded union of the published alternatives. Changing a choice cannot erase an older input/product or turn unrelated freight into general factory storage.
	for (const auto &record : GetMultiLayerManufacturingRecipes()) {
		if (record.layer != MapLayer::Space || record.facility != family->facility) continue;
		if (std::ranges::find(record.recipe.inputs, label, &MultiLayerRecipeCargo::label) != record.recipe.inputs.end()) capacity = std::max(capacity, record.input_capacity);
		if (std::ranges::find(record.recipe.outputs, label, &MultiLayerRecipeCargo::label) != record.recipe.outputs.end()) capacity = std::max(capacity, record.output_capacity);
	}
	return capacity;
}

/** Count actual native packet references rather than cached whole quantities.
 * @param stock Sole physical module compartment.
 * @return Actual packet count across every next-hop bucket in that same container.
 */
static size_t RecipeStockPacketCount(const StationCargoList &stock)
{
	size_t count = 0;
	// Each next-hop bucket owns different real packets; omitting an invalid-route bucket would allow the saved stock-reference bound to overflow.
	for (const auto &[hop, packets] : *stock.Packets()) { (void)hop; count += packets.size(); }
	return count;
}

/** Validate the exact original adjacent electrical source/consumer without allocating or reading a copied balance.
 * @param site Actual operational processing module.
 * @param state Same generation-qualified metadata owning its connection and paid work.
 * @return Whether this original native connection remains owned, active and geometrically valid.
 */
bool IsOrbitalRecipePowerConnectionValid(const SpaceStructure &site, const OrbitalRecipeState &state)
{
	const auto *source = ResolveSpaceStructure(state.source); const auto *consumer = GetUtilityPowerConsumer(state.consumer);
	// A physical adjacent owned panel and this exact existing consumer must supply the same declared processing role; a reused ID or generic service account grants no permission.
	if (source == nullptr || source->state.owner != site.state.owner || GetOrbitalModulePanelRate(*source) == 0 || !AreOrbitalModuleFootprintsAdjacent(site.state, source->state) ||
		consumer == nullptr || !consumer->active || consumer->owner != site.state.owner || consumer->meter.layer != MapLayer::Space || consumer->effect != UtilityConsumerEffect::AssemblyWork || consumer->capacity_j != GetMultiLayerSpaceBudget().work_j_per_tick ||
		consumer->linked_producers.size() != 1 || consumer->linked_producers.front() != source->module->producer) return false;
	return true;
}

/** @copydoc CreateOrbitalRecipeState */
std::unique_ptr<OrbitalRecipeState> CreateOrbitalRecipeState(SpaceStructureRef reference, MultiLayerRecipeID choice, SpaceStructureRef source, UtilityConsumerID consumer)
{
	const auto *site = ResolveSpaceStructure(reference);
	// Only an actual owned operational processor in the active profile/date can bind metadata; constructing a view cannot commission an inert frame or create an electrical identity.
	if (site == nullptr || site->state.owner != _current_company || !IsOrbitalModuleOperational(*site) ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return nullptr;
	const auto *definition = GetOrbitalRecipeDefinition(site->state.kind, choice);
	// A real frame grants no production before its selected content family is available in the synchronized calendar.
	if (definition == nullptr || TimerGameCalendar::year < definition->introduction_year) return nullptr;
	auto state = CreateEmptyOrbitalRecipeState(*site); if (!state) return nullptr;
	state->choice = definition->recipe.id; state->source = source; state->consumer = consumer;
	if (definition->recipe.power_j_per_batch != 0) {
		if (consumer != UtilityConsumerID{UINT32_MAX}) {
			if (!IsOrbitalRecipePowerConnectionValid(*site, *state)) return nullptr;
			state->electrical = std::make_unique<LayerRecipeElectricalWork>(definition->recipe, consumer);
		} else if (source != SpaceStructureRef{}) {
			return nullptr; // An unconnected receipt cannot retain an unexplained original panel claim.
		}
	} else {
		// Genuinely unpowered profile alternatives cannot acquire a hidden connection or pretend an external consumer paid their baseline work.
		if (consumer != UtilityConsumerID{UINT32_MAX} || source != SpaceStructureRef{}) return nullptr;
	}
	return state;
}

/** @copydoc CreateEmptyOrbitalRecipeState */
std::unique_ptr<OrbitalRecipeState> CreateEmptyOrbitalRecipeState(const SpaceStructure &site)
{
	const auto *definition = GetOrbitalRecipeDefinition(site.state.kind);
	// Metadata can describe a real pending activation but cannot complete construction, change ownership or make an inert frame operational.
	if (definition == nullptr || ResolveSpaceStructure(GetSpaceStructureReference(site)) != &site || site.state.phase != SpaceStructurePhase::FrameComplete ||
		site.state.owner != _current_company || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return nullptr;
	auto state = std::make_unique<OrbitalRecipeState>(); state->site = GetSpaceStructureReference(site); state->choice = definition->recipe.id;
	// Preallocate every historical key before any non-failing commit. The metadata owns only counters; all physical packets remain in the original module stock.
	for (const auto &record : GetMultiLayerManufacturingRecipes()) {
		if (record.layer != MapLayer::Space || record.facility != definition->facility) continue;
		state->batches.emplace(record.recipe.id, 0);
		state->residue_mass_sixteenths.emplace(record.recipe.id, 0);
		for (const auto &input : record.recipe.inputs) state->consumed.try_emplace(input.label, 0);
		for (const auto &output : record.recipe.outputs) state->produced.try_emplace(output.label, 0);
	}
	return state;
}

/** @copydoc OrbitalRecipeInventory::Site */
SpaceStructure *OrbitalRecipeInventory::Site() const
{
	auto *site = ResolveSpaceStructure(state.site);
	// Foreign control, lost/reused structure IDs, inactive frames and exhausted revisions invalidate this exact physical owner before any packet or work access.
	if (site == nullptr || site->state.owner != _current_company || !IsOrbitalModuleOperational(*site) || state.revision == 0 || state.revision == UINT64_MAX ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return nullptr;
	const auto *definition = GetOrbitalRecipeDefinition(site->state.kind, state.choice);
	// Profile choice and synchronized introduction date remain binding even after initial metadata creation.
	if (definition == nullptr || TimerGameCalendar::year < definition->introduction_year) return nullptr;
	// Every retained native compartment belongs to this same bounded family. An unrelated or malformed hidden slot must block work as well as later file admission.
	for (CargoType cargo : EnumRange(NUM_CARGO)) {
		const auto &stock = site->module->stock[cargo]; if (stock.TotalCount() == 0) continue;
		if (!IsValidCargoType(cargo) || stock.TotalCount() > GetOrbitalRecipeCargoCapacity(site->state.kind, CargoSpec::Get(cargo)->label) ||
			RecipeStockPacketCount(stock) > MAX_ORBITAL_RECIPE_PACKET_REFERENCES) return nullptr;
	}
	if (definition->recipe.power_j_per_batch != 0) {
		// Electrical work must match its original selected rate; missing receipt framing cannot fall through to a scalar baseline-work claim.
		if (state.unpowered_work_ticks != 0 || (state.electrical && (!ValidateLayerRecipeElectricalWork(definition->recipe, state.electrical->Capture()) || state.electrical->Capture().consumer != state.consumer.base())) ||
			(!state.electrical && state.consumer != UtilityConsumerID{UINT32_MAX})) return nullptr;
	} else {
		// Unpowered alternatives retain bounded synchronized work only; imported electrical metadata cannot expand that budget or another recipe's throughput.
		if (state.electrical || state.consumer != UtilityConsumerID{UINT32_MAX} || state.source != SpaceStructureRef{} ||
			state.unpowered_work_ticks > uint64_t{definition->recipe.work_economy_ticks} * definition->recipe.max_batches) return nullptr;
	}
	return site;
}

/** @copydoc OrbitalRecipeInventory::Capture */
LayerRecipeInventorySnapshot OrbitalRecipeInventory::Capture() const
{
	LayerRecipeInventorySnapshot result; result.recipe = state.choice; result.layer = MapLayer::Space; result.revision = state.revision;
	const auto *site = Site(); if (site == nullptr) return result;
	const auto &definition = *GetOrbitalRecipeDefinition(site->state.kind, state.choice); result.active = true;
	result.work_ticks = definition.recipe.power_j_per_batch != 0 ? (state.electrical ? state.electrical->WorkTicks() : 0) : state.unpowered_work_ticks;
	UtilityGrantID grant;
	if (GetElectricalGrant(grant)) result.power_j = GetUtilityAllocation(grant.consumer)->granted_energy_j;
	// Resolve each canonical meaning against the actual live cargo registry. A missing form or oversized native reference list invalidates the whole view, preserving every original packet.
	for (const auto &input : definition.recipe.inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		if (!IsValidCargoType(cargo)) { result.active = false; return result; }
		const auto &stock = site->module->stock[cargo]; if (RecipeStockPacketCount(stock) > MAX_ORBITAL_RECIPE_PACKET_REFERENCES) { result.active = false; return result; }
		result.inputs.push_back({input.label, stock.AvailableCount(), GetOrbitalRecipeCargoCapacity(site->state.kind, input.label)});
	}
	// Native packet-reference room is a real output bound even when whole-unit space remains; conservatively require one complete new product reference before any input can disappear.
	for (const auto &output : definition.recipe.outputs) {
		const auto cargo = GetCargoTypeByLabel(output.label);
		if (!IsValidCargoType(cargo)) { result.active = false; return result; }
		const auto &stock = site->module->stock[cargo]; const auto quantity = stock.TotalCount(); const auto references = RecipeStockPacketCount(stock);
		if (references > MAX_ORBITAL_RECIPE_PACKET_REFERENCES) { result.active = false; return result; }
		const uint32_t capacity = references == MAX_ORBITAL_RECIPE_PACKET_REFERENCES ? quantity : GetOrbitalRecipeCargoCapacity(site->state.kind, output.label);
		result.outputs.push_back({output.label, quantity, capacity});
	}
	return result;
}

/** @copydoc OrbitalRecipeInventory::GetElectricalGrant */
bool OrbitalRecipeInventory::GetElectricalGrant(UtilityGrantID &grant) const
{
	const auto *site = Site();
	// Only this exact original electrical receipt/connection can expose its current native grant; fully paid material settlement itself needs no fresh source debit.
	if (site == nullptr || !state.electrical || !IsOrbitalRecipePowerConnectionValid(*site, state) || !GetUtilityAllocation(state.consumer)) return false;
	grant = {state.consumer, GetUtilityPowerInterval()}; return true;
}

/** @copydoc OrbitalRecipeInventory::ValidatePhysicalBatch */
bool OrbitalRecipeInventory::ValidatePhysicalBatch(const LayerRecipeBatchProposal &proposal, bool allocated) const
{
	const auto *site = Site(); if (site == nullptr) return false; const auto &definition = GetOrbitalRecipeDefinition(site->state.kind, state.choice)->recipe;
	if (state.revision >= UINT64_MAX - 1) return false;
	// The common pure validator owns all-delta revalidation. Its non-const participant handle only compares the existing mutable receipt pointer; no physical callback or mutation is invoked here.
	auto &participant = const_cast<OrbitalRecipeInventory &>(*this);
	if (ValidateLayerRecipeBatchProposal(definition, *this, participant, proposal) != LayerRecipeCommitError::None || proposal.plan.batches == 0) return false;
	const auto history = state.batches.find(state.choice);
	// Historical receipts must remain representable before consuming another physical batch; wrapping a counter would fabricate fresh material history.
	if (history == state.batches.end() || history->second > UINT64_MAX - proposal.plan.batches) return false;
	const auto residue = state.residue_mass_sixteenths.find(state.choice);
	const uint64_t residue_rate = GetOrbitalRecipeDefinition(site->state.kind, state.choice)->residue_mass_sixteenths;
	// Complete batch residue is a bounded spent-material receipt; an unrepresentable residue cannot be hidden by successfully creating only the freight output.
	if (residue == state.residue_mass_sixteenths.end() || (residue_rate != 0 && proposal.plan.batches > (UINT64_MAX - residue->second) / residue_rate)) return false;
	// Every real input and its preallocated historical counter must fit before any non-failing material debit starts.
	for (const auto &input : proposal.inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label); const auto counter = state.consumed.find(input.label);
		if (!IsValidCargoType(cargo) || counter == state.consumed.end() || counter->second > UINT64_MAX - input.amount || site->module->stock[cargo].AvailableCount() < input.amount) return false;
	}
	// No partial preallocation can authorize the complete material transaction.
	if (allocated && products.size() != proposal.outputs.size()) return false;
	// All product slots, native reference room and historical credits are one complete admission; no earlier input can finance a partially representable output set.
	for (size_t i = 0; i < proposal.outputs.size(); ++i) {
		const auto &output = proposal.outputs[i]; const auto cargo = GetCargoTypeByLabel(output.label); const auto counter = state.produced.find(output.label);
		if (!IsValidCargoType(cargo) || counter == state.produced.end() || counter->second > UINT64_MAX - output.amount || output.amount > UINT16_MAX) return false;
		const auto &stock = site->module->stock[cargo]; const auto capacity = GetOrbitalRecipeCargoCapacity(site->state.kind, output.label);
		if (stock.TotalCount() > capacity || output.amount > capacity - stock.TotalCount() || RecipeStockPacketCount(stock) >= MAX_ORBITAL_RECIPE_PACKET_REFERENCES) return false;
		if (allocated && (products[i].cargo != cargo || products[i].amount != output.amount || !products[i].packet || products[i].packet->Count() != output.amount)) return false;
	}
	return true;
}

/** @copydoc OrbitalRecipeInventory::PrepareOutputs */
bool OrbitalRecipeInventory::PrepareOutputs(const LayerRecipeBatchProposal &proposal, bool execute)
{
	// One synchronous participant owns at most one complete product allocation. Queries cannot destroy an earlier allocation or reserve native packet identities.
	if (!products.empty() || !ValidatePhysicalBatch(proposal, false) || !CargoPacket::CanAllocateItem(proposal.outputs.size()) ||
		(allocation_admission != nullptr && !allocation_admission(proposal.outputs.size()))) return false;
	if (!execute) return true;
	products.reserve(proposal.outputs.size());
	// Allocate every original product first. Explicit native unknown provenance must not impersonate an Industry ID or the asteroid source consumed by this recipe; the actual module ledger records production.
	for (const auto &output : proposal.outputs) {
		auto *packet = CargoPacket::Create(StationID::Invalid(), static_cast<uint16_t>(output.amount), Source{Source::Invalid, SourceType::Industry});
		if (packet == nullptr) { products.clear(); return false; }
		products.push_back({GetCargoTypeByLabel(output.label), output.amount, std::unique_ptr<CargoPacket>{packet}});
	}
	return true;
}

/** @copydoc OrbitalRecipeInventory::Validate */
bool OrbitalRecipeInventory::Validate(const LayerRecipeBatchProposal &proposal) const { return ValidatePhysicalBatch(proposal, true); }

/** @copydoc OrbitalRecipeInventory::Commit */
bool OrbitalRecipeInventory::Commit(const LayerRecipeBatchProposal &proposal) noexcept
{
	if (!Validate(proposal)) return false;
	auto &site = *Site();
	// All distinct input roles and positive quantities were revalidated by the shared definition and actual packet owner. Native deterministic consumption cannot allocate, pay income or fail after this admission.
	for (const auto &input : proposal.inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label); const bool consumed = HubCargoTransfer::Consume(site.module->stock[cargo], input.amount); assert(consumed); (void)consumed;
		state.consumed.find(input.label)->second += input.amount;
	}
	// Every real output already exists in this participant's temporary ownership; appending transfers that same packet once into the original native module compartment.
	for (auto &product : products) site.module->stock[product.cargo].Append(product.packet.release(), StationID::Invalid());
	for (const auto &output : proposal.outputs) state.produced.find(output.label)->second += output.amount;
	if (!state.electrical) state.unpowered_work_ticks -= proposal.work_ticks;
	state.residue_mass_sixteenths.find(state.choice)->second += uint64_t{GetOrbitalRecipeDefinition(site.state.kind, state.choice)->residue_mass_sixteenths} * proposal.plan.batches;
	state.batches.find(state.choice)->second += proposal.plan.batches; ++state.revision; products.clear(); return true;
}

/** @copydoc OrbitalRecipeInventory::CanRecordWork */
bool OrbitalRecipeInventory::CanRecordWork(uint64_t energy_j) const
{
	const auto *site = Site();
	// Keep a physical revision available for complete settlement and refuse stale stock/work before the shared utility owner spends any source energy.
	if (site == nullptr || state.revision >= UINT64_MAX - 2 || energy_j > UINT64_MAX - state.paid_lifetime_j) return false;
	const auto &recipe = GetOrbitalRecipeDefinition(site->state.kind, state.choice)->recipe; const auto batch = state.batches.find(state.choice);
	if (batch == state.batches.end() || batch->second == UINT64_MAX) return false;
	const auto residue = state.residue_mass_sixteenths.find(state.choice);
	const auto residue_rate = GetOrbitalRecipeDefinition(site->state.kind, state.choice)->residue_mass_sixteenths;
	// Do not spend actual source energy ahead of an unrepresentable future spent-material residue receipt.
	if (residue == state.residue_mass_sixteenths.end() || residue->second > UINT64_MAX - residue_rate) return false;
	// Work cannot pay ahead of an unrepresentable future input/output receipt, even when the actual electrical grant and packet headroom are otherwise sufficient.
	for (const auto &input : recipe.inputs) {
		const auto counter = state.consumed.find(input.label); if (counter == state.consumed.end() || counter->second > UINT64_MAX - input.quantity) return false;
	}
	for (const auto &output : recipe.outputs) {
		const auto counter = state.produced.find(output.label); if (counter == state.produced.end() || counter->second > UINT64_MAX - output.quantity) return false;
	}
	return true;
}

/** @copydoc OrbitalRecipeInventory::ValidateWork */
bool OrbitalRecipeInventory::ValidateWork(const LayerRecipeWorkProposal &proposal) const
{
	return EqualLayerRecipeInventory(proposal.before, Capture()) && CanRecordWork(proposal.energy_j);
}

/** @copydoc OrbitalRecipeInventory::CommitWork */
bool OrbitalRecipeInventory::CommitWork(const LayerRecipeWorkProposal &proposal) noexcept
{
	if (!ValidateWork(proposal)) return false;
	state.paid_lifetime_j += proposal.energy_j; ++state.revision; return true;
}
