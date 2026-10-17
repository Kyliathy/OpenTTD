/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_recipe_record.cpp Independent same-file processing owners and exact material/paid-work history.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C
 * This validator owns no packets, electrical accounts or runtime progress; private restore follows complete admission.
 */
#include "stdafx.h"
#include "orbital_recipe.h"
#include "space_structure.h"
#include "timer/timer_game_tick.h"
#include "safeguards.h"

/** Accumulate one checked product without allowing history arithmetic to wrap.
 * @param total Existing exact aggregate. @param count Complete historical batch count. @param quantity Immutable per-batch quantity.
 * @return Whether the exact accumulated historical quantity remains representable.
 */
static bool AddRecipeHistory(uint64_t &total, uint64_t count, uint64_t quantity)
{
	// A wrapped history could authorize fresh physical stock or hide spent power; admit the whole product before changing this local validation aggregate.
	if (quantity != 0 && count > (UINT64_MAX - total) / quantity) return false;
	total += count * quantity; return true;
}

/** @param id Original canonical recipe label. @return Published supported processing definition, with no locally copied numerical recipe. */
static const MultiLayerManufacturingRecipe *HistoricalOrbitalRecipe(MultiLayerRecipeID id)
{
	if (id == MultiLayerRecipeID{}) return nullptr;
	// Each canonical native module family is searched through the original content owner; a different layer or arbitrary recipe label cannot become retired orbital history.
	for (const auto kind : {SpaceBuildKind::Steelworks, SpaceBuildKind::Goodsworks, SpaceBuildKind::Refinery, SpaceBuildKind::IceProcessor}) {
		if (const auto *definition = GetOrbitalRecipeDefinition(kind, id)) return definition;
	}
	return nullptr;
}

/** @copydoc GetOrbitalRecipeRetirementEnergy */
std::optional<uint64_t> GetOrbitalRecipeRetirementEnergy(const SpaceStructureRetirementHistory &history)
{
	// A pristine native structure authority cannot import another world's spent processing history.
	if (history.structures_removed == 0 && (!history.processing_batches.empty() || history.processing_discarded_paid_j != 0)) return std::nullopt;
	uint64_t power = history.processing_discarded_paid_j, residue = 0; std::map<CargoLabel, uint64_t> consumed, produced;
	// Every retired recipe still owns its exact original material/residue and spent-power bill. Aggregate overflow must refuse demolition before any native owner disappears.
	for (const auto &[id, batches] : history.processing_batches) {
		const auto *definition = HistoricalOrbitalRecipe(id);
		if (definition == nullptr || batches == 0 || !AddRecipeHistory(power, batches, definition->recipe.power_j_per_batch) || !AddRecipeHistory(residue, batches, definition->residue_mass_sixteenths)) return std::nullopt;
		for (const auto &input : definition->recipe.inputs) if (!AddRecipeHistory(consumed[input.label], batches, input.quantity)) return std::nullopt;
		for (const auto &output : definition->recipe.outputs) if (!AddRecipeHistory(produced[output.label], batches, output.quantity)) return std::nullopt;
	}
	return power;
}

/** Reproduce exact per-cargo consumed/produced history and already spent power from original complete batch counts.
 * @param record Detached actual processing metadata.
 * @param kind Original same-file processor role.
 * @param completed_power Receives only the exact original complete-batch electrical bill.
 * @return Whether original family keys and every historical cargo debit/credit match exactly.
 */
static bool ValidateRecipeMaterialHistory(const OrbitalRecipeRecord &record, SpaceBuildKind kind, uint64_t &completed_power)
{
	const auto *family = GetOrbitalRecipeDefinition(kind, record.choice); if (family == nullptr) return false;
	std::map<MultiLayerRecipeID, uint64_t> expected_batches;
	std::map<MultiLayerRecipeID, uint64_t> residue;
	std::map<CargoLabel, uint64_t> consumed, produced;
	// Every family alternative contributes its own immutable all-input/output quantities; changing a saved choice cannot reinterpret an earlier batch or erase its residue/material history.
	for (const auto &definition : GetMultiLayerManufacturingRecipes()) {
		if (definition.layer != MapLayer::Space || definition.facility != family->facility) continue;
		const auto found = record.batches.find(definition.recipe.id); if (found == record.batches.end()) return false;
		expected_batches.emplace(found->first, found->second);
		if (!AddRecipeHistory(residue[found->first], found->second, definition.residue_mass_sixteenths)) return false;
		if (!AddRecipeHistory(completed_power, found->second, definition.recipe.power_j_per_batch)) return false;
		for (const auto &input : definition.recipe.inputs) if (!AddRecipeHistory(consumed[input.label], found->second, input.quantity)) return false;
		for (const auto &output : definition.recipe.outputs) if (!AddRecipeHistory(produced[output.label], found->second, output.quantity)) return false;
	}
	return expected_batches == record.batches && consumed == record.consumed && produced == record.produced && residue == record.residue_mass_sixteenths;
}

/** @copydoc CaptureOrbitalRecipes */
std::vector<OrbitalRecipeRecord> CaptureOrbitalRecipes()
{
	std::vector<OrbitalRecipeRecord> rows;
	// Copy only existing native metadata/receipt fields in deterministic pool order. The same original module remains sole stock owner throughout.
	for (const auto *site : SpaceStructure::Iterate()) {
		if (!site->module || !site->module->processing) continue;
		const auto &state = *site->module->processing; OrbitalRecipeRecord row;
		static_cast<OrbitalRecipeMetadata &>(row) = state;
		if (state.electrical) row.electrical = state.electrical->Capture();
		rows.push_back(std::move(row));
	}
	return rows;
}

/** @copydoc ValidateOrbitalRecipeRecords */
bool ValidateOrbitalRecipeRecords(std::span<const OrbitalRecipeRecord> records, std::span<const SpaceStructureRecord> sites,
	const UtilityPowerState &utility, uint64_t native_tick, const SpaceStructureRetirementHistory &history)
{
	const auto retired_power = GetOrbitalRecipeRetirementEnergy(history); if (!retired_power) return false;
	uint64_t physical_paid = *retired_power, source_paid = 0;
	// Retired source identities retain actual spent history. Summing real native panels bounds the entire processor claim without introducing another electrical balance.
	for (const auto &producer : utility.producers) {
		if (producer.kind == UtilityGenerationKind::OrbitalPanel && !AddRecipeHistory(source_paid, producer.consumed_lifetime_j, 1)) return false;
	}
	std::set<UtilityConsumerID> consumers;
	// Processing consumers cannot alias any original service, Core construction or Assembly work account, even if their declared generic utility effect is the same.
	for (const auto &site : sites) if (site.module) {
		for (const auto id : {site.module->consumer, site.module->assembly_consumer}) if (id != UtilityConsumerID{UINT32_MAX}) consumers.insert(id);
	}
	size_t next = 0;
	// Require exactly one canonical processing owner for every same-file processor module. Extra records, missing modules and reordered/duplicate identities cannot borrow another incarnation's progress.
	for (const auto &site : sites) {
		if (!site.module || !IsOrbitalRecipeKind(site.state.kind)) continue;
		if (next == records.size()) return false;
		const auto &record = records[next++]; const auto *definition = GetOrbitalRecipeDefinition(site.state.kind, record.choice);
		if (record.site != site.ref || record.choice == MultiLayerRecipeID{} || definition == nullptr || record.revision == 0 || record.revision == UINT64_MAX ||
			(record.last_native_tick != 0 && record.last_native_tick - 1 > native_tick) || record.unpowered_work_ticks > definition->recipe.work_economy_ticks) return false;
		uint64_t completed_power = 0;
		if (!ValidateRecipeMaterialHistory(record, site.state.kind, completed_power)) return false;
		/** @param entry Exact canonical recipe/batch pair. @return Whether this owner retains any completed processing history. */
		const bool has_batches = std::ranges::any_of(record.batches, [](const auto &entry) { return entry.second != 0; });
		// Partial activation cannot inherit completed work/history. Every genuinely worked module also retains a real synchronized native opportunity stamp.
		if ((site.module->activation_ticks != 0 && (record.enabled || record.last_native_tick != 0 || record.unpowered_work_ticks != 0 || record.paid_lifetime_j != 0 || has_batches ||
			record.choice != GetDefaultOrbitalRecipe(site.state.kind) || record.electrical || record.consumer != UtilityConsumerID{UINT32_MAX} || record.source != SpaceStructureRef{})) ||
			(record.last_native_tick == 0 && (record.unpowered_work_ticks != 0 || record.paid_lifetime_j != 0 || has_batches))) return false;
		uint64_t accounted = completed_power;
		if (!AddRecipeHistory(accounted, record.discarded_paid_j, 1)) return false;
		if (definition->recipe.power_j_per_batch == 0) {
			// A genuinely unpowered alternative cannot retain an electrical connection, paid receipt or hidden historical electrical budget.
			if (record.electrical || record.consumer != UtilityConsumerID{UINT32_MAX} || record.source != SpaceStructureRef{} || record.paid_lifetime_j != 0 || record.discarded_paid_j != 0) return false;
		} else {
			if (record.unpowered_work_ticks != 0) return false;
			if (record.electrical) {
				const auto &work = *record.electrical;
				// Exact immutable work and same-file epoch precede actual source/consumer joins; future opportunities or another consumer cannot create native paid progress.
				if (!ValidateLayerRecipeElectricalWork(definition->recipe, work) || work.consumer != record.consumer.base() || work.last_interval > utility.interval || record.paid_lifetime_j < work.interval_paid_j ||
					!AddRecipeHistory(accounted, work.paid_j, 1) || !consumers.insert(record.consumer).second) return false;
				if (record.consumer.base() >= utility.consumers.size()) return false;
				const auto &consumer = utility.consumers[record.consumer.base()]; const SpaceStructureRecord *source = nullptr;
				// Resolve the complete original source incarnation from this file's catalogue; another live world or a reused numeric ID supplies no connection rights.
				for (const auto &candidate : sites) if (candidate.ref == record.source) { source = &candidate; break; }
				// The exact bounded native consumer and original owned physical panel must remain one same-file connection, including retained inactive company-loss history.
				if (consumer.meter.consumer_id != record.consumer || consumer.meter.layer != MapLayer::Space || consumer.meter.domain != UtilityEnergyDomain::Electricity ||
					consumer.effect != UtilityConsumerEffect::AssemblyWork || consumer.capacity_j != definition->recipe.power_j_per_batch / definition->recipe.work_economy_ticks ||
					source == nullptr || !source->module || source->state.owner != site.state.owner || source->state.phase != SpaceStructurePhase::FrameComplete || source->module->activation_ticks != 0 ||
					(source->state.kind != SpaceBuildKind::Core && source->state.kind != SpaceBuildKind::Power) || !AreOrbitalModuleFootprintsAdjacent(site.state, source->state) ||
					(consumer.active ? consumer.linked_producers.size() != 1 || consumer.linked_producers.front() != source->module->producer : !consumer.linked_producers.empty()) ||
					source->module->producer.base() >= utility.producers.size()) return false;
				const auto &producer = utility.producers[source->module->producer.base()];
				// Lost companies retain spent receipts but can never revive an active reservation by reusing their old numeric company identity.
				if (producer.meter.producer_id != source->module->producer || producer.kind != UtilityGenerationKind::OrbitalPanel || producer.meter.layer != MapLayer::Space ||
					(site.state.owner == OWNER_NONE ? consumer.active || producer.active : consumer.owner != site.state.owner || producer.owner != site.state.owner)) return false;
				if (work.last_interval == utility.interval) {
					uint64_t spent = 0;
					// Current pending/settled opportunity use is backed by this exact consumer's actual same-file settled shares, never its unused scalar grant.
					for (const auto &share : consumer.shares) if (!AddRecipeHistory(spent, share.consumed_j, 1)) return false;
					if (work.interval_paid_j > spent) return false;
				}
			} else if (record.consumer != UtilityConsumerID{UINT32_MAX} || record.source != SpaceStructureRef{}) {
				return false;
			}
		}
		// One actual native owner must account for every spent joule as completed, pending or explicitly discarded physical work; no component can silently refund or duplicate it.
		if (accounted != record.paid_lifetime_j || !AddRecipeHistory(physical_paid, record.paid_lifetime_j, 1)) return false;
	}
	return next == records.size() && physical_paid <= source_paid;
}

/** @copydoc ValidateOrbitalRecipes */
bool ValidateOrbitalRecipes()
{
	return ValidateOrbitalRecipeRecords(CaptureOrbitalRecipes(), CaptureSpaceStructures(), CaptureUtilityPowerState(), TimerGameTick::counter, GetSpaceStructureRetirementHistory());
}

/** @copydoc ComputeOrbitalRecipeDigest */
uint64_t ComputeOrbitalRecipeDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	/** @param value Original canonical unsigned scalar, hashed in stable big-endian bytes. */
	auto field = [&](uint64_t value) { for (int shift = 56; shift >= 0; shift -= 8) { digest ^= static_cast<uint8_t>(value >> shift); digest *= 1099511628211ULL; } };
	/** @param value Exact original four-byte label; no runtime ordinal enters this digest. */
	auto label = [&](const BaseLabel &value) { for (const auto byte : value) field(byte); };
	/** @param values Every original bounded canonical history key and quantity, in stable sorted order. */
	auto history = [&](const auto &values) {
		field(values.size());
		// Key meaning and quantity must both participate: a changed recipe/cargo history cannot hide behind an unchanged aggregate total.
		for (const auto &[key, quantity] : values) { label(key); field(quantity); }
	};
	const auto &retired = GetSpaceStructureRetirementHistory(); history(retired.processing_batches); field(retired.processing_discarded_paid_j);
	const auto rows = CaptureOrbitalRecipes(); field(rows.size());
	// Hash every actual metadata/private receipt field once; GUI queries, cancellation/restart and own resaves cannot conceal renewed progress or a different native owner.
	for (const auto &row : rows) {
		field(row.site.structure.base()); field(row.site.generation); label(row.choice); field(row.source.structure.base()); field(row.source.generation); field(row.consumer.base());
		field(row.revision); field(row.unpowered_work_ticks); field(row.enabled); field(row.last_native_tick); field(row.paid_lifetime_j); field(row.discarded_paid_j);
		history(row.batches); history(row.consumed); history(row.produced); history(row.residue_mass_sixteenths); field(row.electrical.has_value());
		if (row.electrical) {
			const auto &work = *row.electrical; label(work.recipe); field(work.consumer); field(work.work_per_batch); field(work.power_per_batch); field(work.paid_j);
			field(work.sequence); field(work.last_interval); field(work.interval_paid_j);
		}
	}
	return digest;
}
