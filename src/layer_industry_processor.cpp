/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_industry_processor.cpp Native material processing over the existing Industry input/output inventories. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Groups 10/16B. @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group39B shared pure snapshot. */

#include "stdafx.h"
#include "geothermal_collector.h"
#include "layer_industry_processor.h"
#include "layer_processor_presentation.h"
#include "layer_industry_baseline.h"
#include "industry.h"
#include "company_base.h"
#include "economy_func.h"
#include "openttd.h"
#include "debug.h"

#include "safeguards.h"

static std::map<IndustryID, LayerProcessorState> _layer_processors; ///< Choice/work/history only; physical cargo stays in Industry.
static std::optional<std::vector<LayerProcessorState>> _staged_layer_processors; ///< Unpublished loader candidate; no stock is stored here.

/** @copydoc IsLayerProcessorDefinition */
bool IsLayerProcessorDefinition(IndustryType type)
{
	const auto *capability = GetLayerIndustryCapability(type);
	// Only Surface recipe families own this processing state. Input-only Deep
	// collectors have no mineral output either, but their delivered construction
	// and thermal source readiness belong to geothermal rather than this executor.
	return capability != nullptr && capability->resource == CT_INVALID && capability->layers.Test(MapLayer::Surface) && capability->recipe != MultiLayerRecipeID{};
}

/**
 * Resolve one frozen alternative by the actual native facility's primary identity.
 * @param type Actual mapped facility type.
 * @param choice Frozen alternative ID.
 * @return Supported immutable Surface record, or nullptr; translated industry names never participate.
 */
static const MultiLayerManufacturingRecipe *FindLayerProcessorRecipe(IndustryType type, MultiLayerRecipeID choice)
{
	const auto *capability = GetLayerIndustryCapability(type);
	if (capability == nullptr) return nullptr;
	if (capability->resource != CT_INVALID) return nullptr;
	const auto records = GetMultiLayerManufacturingRecipes();
	const auto primary = std::ranges::find(records, capability->recipe, [](const auto &item) { return item.recipe.id; });
	if (primary == records.end()) return nullptr;
	// Facility membership comes from the content owner, not a runtime label slot or a translated display name.
	for (const auto &record : records) {
		// A saved choice must belong to this physical Surface family; matching product labels do not grant another facility's recipe.
		if (record.recipe.id == choice && record.facility == primary->facility && record.layer == MapLayer::Surface) return &record;
	}
	return nullptr;
}

/** @copydoc GetLayerProcessorState */
const LayerProcessorState *GetLayerProcessorState(IndustryID industry)
{
	const auto found = _layer_processors.find(industry);
	return found == _layer_processors.end() ? nullptr : &found->second;
}

/** @copydoc GetLayerProcessorRecipe */
const MultiLayerManufacturingRecipe *GetLayerProcessorRecipe(const Industry &industry)
{
	const auto *state = GetLayerProcessorState(industry.index);
	return state == nullptr ? nullptr : FindLayerProcessorRecipe(industry.type, state->recipe);
}

/** @copydoc RegisterLayerProcessorIndustry */
void RegisterLayerProcessorIndustry(const Industry &industry)
{
	// Registration belongs to new inventory creation; repeating it must never reset saved control, work or material history.
	if (!IsLayerProcessorDefinition(industry.type) || _layer_processors.contains(industry.index)) return;
	LayerProcessorState state;
	state.industry = industry.index;
	state.controller = Company::IsValidID(industry.founder) ? CompanyID{industry.founder} : CompanyID::Invalid(); // Editor/deity funding preserves founder history without saving a nonexistent company controller.
	state.recipe = GetLayerIndustryCapability(industry.type)->recipe;
	const auto *primary = FindLayerProcessorRecipe(industry.type, state.recipe);
	assert(primary != nullptr);
	// Allocate every historical key before processing can begin; no recipe commit may allocate after its first stock write.
	for (const auto &record : GetMultiLayerManufacturingRecipes()) {
		if (record.facility != primary->facility) continue;
		state.batches.emplace(record.recipe.id, 0);
		for (const auto &input : record.recipe.inputs) state.consumed.emplace(input.label, 0);
		for (const auto &output : record.recipe.outputs) state.produced.emplace(output.label, 0);
	}
	_layer_processors.insert_or_assign(industry.index, std::move(state));
}

/** @copydoc ReleaseLayerProcessorIndustry */
void ReleaseLayerProcessorIndustry(IndustryID industry) { _layer_processors.erase(industry); }
/** @copydoc ResetLayerProcessorState */
void ResetLayerProcessorState()
{
	_layer_processors.clear();
	_staged_layer_processors.reset();
	ResetLayerProcessorLoadStaging();
}

/** @copydoc CaptureLayerProcessors */
std::vector<LayerProcessorState> CaptureLayerProcessors()
{
	std::vector<LayerProcessorState> result;
	result.reserve(_layer_processors.size());
	// Save canonical real-industry order, retaining explicit zero keys needed by later non-allocating commits.
	for (const auto &[id, state] : _layer_processors) result.push_back(state);
	return result;
}

/** @copydoc StageLoadedLayerProcessors */
void StageLoadedLayerProcessors(std::vector<LayerProcessorState> rows)
{
	_staged_layer_processors = std::move(rows);
}

/**
 * Check the one inventory owner's exact saved choice, role keys and all historical batch arithmetic.
 * @param state Unpublished saved side record.
 * @param industry Actual restored stock owner.
 * @return Whether the whole family can activate without inventing keys, cargo or committed batches.
 */
static bool IsLoadedLayerProcessorValid(const LayerProcessorState &state, const Industry &industry)
{
	/** Diagnose a refused candidate without repairing saved material. @param reason Failed ownership/conservation boundary. @return False. */
	const auto refuse = [&](std::string_view reason) {
		Debug(Facility::Misc, Severity::Error, "MLVS_PROCESSOR_REFUSED industry={} type={} recipe={} reason={}", state.industry.base(), industry.type, state.recipe.AsString(), reason);
		return false;
	};
	const auto *record = FindLayerProcessorRecipe(industry.type, state.recipe);
	// Only a known choice on its own physical Surface facility can activate; abandoned control retains stocks/choice.
	if (record == nullptr || !IsValidTile(industry.location.tile) || GetTileLayer(industry.location.tile) != record->layer ||
			(state.controller != CompanyID::Invalid() && !Company::IsValidID(state.controller)) || state.revision == 0 ||
			state.scale_fraction >= 200 || state.work_ticks > static_cast<uint64_t>(record->recipe.work_economy_ticks) * record->recipe.max_batches) return refuse("choice/layer/controller/work");
	std::map<MultiLayerRecipeID, uint64_t> batches;
	std::map<CargoLabel, uint64_t> consumed, produced;
	// Recompute the immutable role sets and totals from complete committed batches, including unused alternatives' zero keys.
	for (const auto &candidate : GetMultiLayerManufacturingRecipes()) {
		if (candidate.facility != record->facility) continue;
		const auto count = state.batches.find(candidate.recipe.id);
		if (count == state.batches.end()) return refuse("missing batch key");
		batches.emplace(count->first, count->second);
		// Checked multiply/add refuses wrapped historical consumption rather than creating valid-looking renewed stock.
		for (const auto &role : candidate.recipe.inputs) {
			auto &total = consumed[role.label];
			if (count->second > (UINT64_MAX - total) / role.quantity) return false;
			total += count->second * role.quantity;
		}
		// Every product counter must equal the frozen batch credits, even when all physical product has already left.
		for (const auto &role : candidate.recipe.outputs) {
			auto &total = produced[role.label];
			if (count->second > (UINT64_MAX - total) / role.quantity) return false;
			total += count->second * role.quantity;
		}
	}
	if (batches != state.batches || consumed != state.consumed || produced != state.produced) return refuse("historical batch arithmetic/keys"); // Exact key sets exclude foreign counters and missing preallocated commit keys.
	std::set<CargoLabel> inputs, outputs;
	// Every actual incoming slot belongs to the installed alternatives' union, once, with bounded retained delivery.
	for (const auto &slot : industry.accepted) {
		if (!IsValidCargoType(slot.cargo)) return false;
		const CargoLabel label = CargoSpec::Get(slot.cargo)->label;
		if (!consumed.contains(label) || slot.waiting > record->input_capacity || !inputs.insert(label).second) return refuse(fmt::format("input slot {} stock {}", label.AsString(), slot.waiting)); // A changed cargo meaning cannot consume another input's saved history.
	}
	uint64_t total_produced = 0, total_stock = 0;
	// Physical output plus exact historical dispatch must account for all products from the recorded batches.
	for (const auto &slot : industry.produced) {
		if (!IsValidCargoType(slot.cargo)) return false;
		const CargoLabel label = CargoSpec::Get(slot.cargo)->label;
		if (!produced.contains(label) || slot.waiting > record->output_capacity || !outputs.insert(label).second) return refuse(fmt::format("output slot {} stock {}", label.AsString(), slot.waiting)); // No duplicate, foreign or overfilled product slot may activate.
		if (produced.at(label) > UINT64_MAX - total_produced) return false;
		total_produced += produced.at(label);
		total_stock += slot.waiting;
	}
	// Exact unions prohibit silently dropping a delivered alternative on load; dispatch never owns another inventory.
	if (inputs.size() != consumed.size() || outputs.size() != produced.size() || state.dispatched > total_produced ||
			total_produced - state.dispatched != total_stock) return refuse(fmt::format("union/material inputs {}/{} outputs {}/{} produced {} dispatched {} stock {}", inputs.size(), consumed.size(), outputs.size(), produced.size(), total_produced, state.dispatched, total_stock));
	return true;
}

/** @copydoc AcceptLoadedLayerProcessors */
bool AcceptLoadedLayerProcessors()
{
	const auto rows = _staged_layer_processors ? std::move(*_staged_layer_processors) : CaptureLayerProcessors();
	_staged_layer_processors.reset();
	if (rows.size() > 64000) return false;
	std::map<IndustryID, LayerProcessorState> accepted;
	// Publish only after every real owner/choice/role/conservation check passes; never repair a partial family.
	for (const auto &state : rows) {
		const Industry *industry = Industry::GetIfValid(state.industry);
		if (industry == nullptr) return false;
		if (!IsLoadedLayerProcessorValid(state, *industry)) return false;
		if (!accepted.emplace(state.industry, state).second) return false;
	}
	// Missing rows cannot turn a native saved industry into an unaccounted legacy converter.
	for (const Industry *industry : Industry::Iterate()) {
		if (IsLayerProcessorDefinition(industry->type) && !accepted.contains(industry->index)) return false;
	}
	_layer_processors = std::move(accepted);
	Debug(Facility::Misc, Severity::Info, "MLVS_PROCESSORS_LOADED count={} digest={:016X}", _layer_processors.size(), ComputeLayerProcessorDigest());
	return true;
}

/** @copydoc ComputeLayerProcessorDigest */
uint64_t ComputeLayerProcessorDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	/** Append explicit integer bytes in stable byte order, independent of host padding. @param value Actual owner field. */
	const auto add = [&digest](uint64_t value) {
		for (int shift = 56; shift >= 0; shift -= 8) {
			digest ^= static_cast<uint8_t>(value >> shift);
			digest *= 1099511628211ULL;
		}
	};
	// Counter categories, key counts and canonical map order distinguish saved choices and all material history.
	for (const auto &[id, state] : _layer_processors) {
		for (uint64_t value : {uint64_t{id.base()}, uint64_t{state.controller.base()}, state.work_ticks, uint64_t{state.scale_fraction}, state.revision, state.dispatched}) add(value);
		for (uint8_t byte : state.recipe) add(byte);
		add(state.batches.size());
		for (const auto &[key, value] : state.batches) { for (uint8_t byte : key) add(byte); add(value); }
		add(state.consumed.size());
		for (const auto &[key, value] : state.consumed) { for (uint8_t byte : key) add(byte); add(value); }
		add(state.produced.size());
		for (const auto &[key, value] : state.produced) { for (uint8_t byte : key) add(byte); add(value); }
		const Industry *industry = Industry::GetIfValid(id);
		if (industry == nullptr) continue;
		// Stock is read from its real owner; labels avoid runtime cargo-slot rearrangement in diagnostics.
		for (const auto &slot : industry->accepted) { for (uint8_t byte : CargoSpec::Get(slot.cargo)->label) add(byte); add(slot.waiting); }
		for (const auto &slot : industry->produced) { for (uint8_t byte : CargoSpec::Get(slot.cargo)->label) add(byte); add(slot.waiting); }
	}
	return digest;
}

/** @copydoc ChangeLayerProcessorControl */
void ChangeLayerProcessorControl(CompanyID old_company, CompanyID new_company)
{
	// Transfer recipe control independently of the immutable founding history; commercial inventories keep their physical owner.
	for (auto &[id, state] : _layer_processors) {
		if (state.controller != old_company) continue;
		state.controller = new_company;
		if (state.revision != UINT64_MAX) ++state.revision; // Exhausted revisions remain inert rather than wrapping into a reusable quote.
	}
}

/** @copydoc LayerProcessorHasStoredCargo */
bool LayerProcessorHasStoredCargo(const Industry &industry)
{
	if (!IsLayerProcessorDefinition(industry.type)) return false;
	return std::ranges::any_of(industry.accepted, [](const auto &slot) { return slot.waiting != 0; }) ||
		std::ranges::any_of(industry.produced, [](const auto &slot) { return slot.waiting != 0; });
}

/** @copydoc GetLayerProcessorInputRoom */
uint32_t GetLayerProcessorInputRoom(const Industry &industry, CargoType cargo)
{
	const auto *state = GetLayerProcessorState(industry.index);
	const auto *recipe = GetLayerProcessorRecipe(industry);
	// Unbound/restoring, unavailable or malformed facilities never advertise physical intake room.
	if (state == nullptr || recipe == nullptr || state->revision == UINT64_MAX || !IsValidCargoType(cargo)) return 0;
	if (!IsLayerProcessorActive(industry, *recipe)) return 0;
	const auto slot = std::ranges::find(industry.accepted, cargo, &Industry::AcceptedCargo::cargo);
	if (slot == industry.accepted.end()) return 0;
	if (slot->waiting >= recipe->input_capacity) return 0;
	const uint32_t accepted = slot->history == nullptr ? 0 : (*slot->history)[THIS_MONTH].accepted;
	return std::min<uint32_t>(recipe->input_capacity - slot->waiting, UINT16_MAX - accepted);
}

/** @copydoc ReceiveLayerProcessorCargo */
uint32_t ReceiveLayerProcessorCargo(Industry &industry, CargoType cargo, uint32_t offered)
{
	const uint32_t amount = std::min(offered, GetLayerProcessorInputRoom(industry, cargo));
	if (amount == 0) return 0;
	auto slot = std::ranges::find(industry.accepted, cargo, &Industry::AcceptedCargo::cargo);
	auto &history = slot->GetOrCreateHistory(); // Finish the only possible allocation before the first physical stock write.
	slot->waiting += static_cast<uint16_t>(amount);
	history[THIS_MONTH].accepted += static_cast<uint16_t>(amount);
	slot->last_accepted = TimerGameEconomy::date;
	++_layer_processors.at(industry.index).revision;
	return amount;
}

/** @copydoc GetLayerIndustryReceiverRoomAt */
uint32_t GetLayerIndustryReceiverRoomAt(const Station &station, CargoType cargo)
{
	if (!IsValidCargoType(cargo)) return 0;
	if (station.always_accepted.Test(cargo)) return UINT32_MAX;
	uint64_t room = 0;
	// Actual catchment and exclusivity determine physical receivers; a coincident building on another layer supplies no room.
	for (const auto &near : station.industries_near) {
		const Industry &industry = *near.industry;
		if (GetTileLayer(industry.location.tile) != GetTileLayer(station.xy)) continue;
		if (industry.exclusive_supplier != INVALID_OWNER && industry.exclusive_supplier != station.owner) continue;
		const auto slot = std::ranges::find(industry.accepted, cargo, &Industry::AcceptedCargo::cargo);
		if (slot == industry.accepted.end()) continue;
		if (IsGeothermalCollectorDefinition(industry.type)) {
			room += GetGeothermalCollectorInputRoom(industry, cargo); // Actual carrier unloading must respect the one remaining construction bill.
		} else if (IsLayerProcessorDefinition(industry.type)) {
			room += GetLayerProcessorInputRoom(industry, cargo);
		} else if (IsLayerBaselineSteelDefinition(industry.type)) {
			room += GetLayerBaselineSteelInputRoom(industry, cargo);
		} else {
			room += UINT16_MAX - slot->waiting;
		}
	}
	return static_cast<uint32_t>(std::min<uint64_t>(room, UINT32_MAX));
}

/** Actual native facility adapter implements the common all-input view/commit boundary without copying cargo into another owner. */
class NativeLayerProcessorInventory : public LayerRecipeInventoryView, public LayerRecipeCommitParticipant {
	Industry &industry; ///< Sole physical stock owner.
	LayerProcessorState &state; ///< Choice, work and historical counts only.
	const MultiLayerManufacturingRecipe &record; ///< Frozen selected recipe/caps.

public:
	/** @param industry Existing physical facility. @param state Its validated side record. @param record Current frozen choice. */
	NativeLayerProcessorInventory(Industry &industry, LayerProcessorState &state, const MultiLayerManufacturingRecipe &record) : industry(industry), state(state), record(record) {}
	/** @return Pure actual stock/work/choice snapshot; this material adapter has no utility grants. */
	LayerRecipeInventorySnapshot Capture() const override
	{
		return CaptureLayerProcessorInventory(this->industry, this->state, this->record);
	}
	/** @return This exact actual inventory view. */
	const LayerRecipeInventoryView &Inventory() const override { return *this; }
	/** @param proposal Complete shared-executor proposal. @return Whether actual slots and every historical counter still accept the whole commit. */
	bool Validate(const LayerRecipeBatchProposal &proposal) const override
	{
		if (!IsLayerProcessorActive(this->industry, this->record)) return false;
		// Saved choice/revision/work and the batch counter must admit this exact transaction without narrow or wide wrap.
		if (proposal.before.revision != this->state.revision || proposal.before.recipe != this->state.recipe ||
				proposal.work_ticks > this->state.work_ticks || proposal.plan.batches > UINT64_MAX - this->state.batches.at(this->state.recipe)) return false;
		// Every delivered input remains in the sole physical Industry inventory until all other participants pass.
		for (const auto &delta : proposal.inputs) {
			const CargoType cargo = GetCargoTypeByLabel(delta.label);
			const auto slot = std::ranges::find(this->industry.accepted, cargo, &Industry::AcceptedCargo::cargo);
			if (slot == this->industry.accepted.end()) return false;
			if (slot->waiting < delta.amount) return false;
			if (delta.amount > UINT64_MAX - this->state.consumed.at(delta.label)) return false;
		}
		// Every output space and production counter must fit before any input is consumed.
		for (const auto &delta : proposal.outputs) {
			const CargoType cargo = GetCargoTypeByLabel(delta.label);
			const auto slot = std::ranges::find(this->industry.produced, cargo, &Industry::ProducedCargo::cargo);
			if (slot == this->industry.produced.end()) return false;
			if (slot->waiting > this->record.output_capacity) return false;
			if (delta.amount > static_cast<uint64_t>(this->record.output_capacity - slot->waiting)) return false;
			if (delta.amount > static_cast<uint64_t>(UINT16_MAX - slot->history[THIS_MONTH].production)) return false;
			if (delta.amount > UINT64_MAX - this->state.produced.at(delta.label)) return false;
		}
		return true;
	}
	/** @param proposal Fully revalidated common batch. @return Complete non-allocating stock/work/history commit or unchanged refusal. */
	bool Commit(const LayerRecipeBatchProposal &proposal) noexcept override
	{
		if (!this->Validate(proposal)) return false;
		// All lookups, capacities and counters passed: debit each input and credit every product together, without callbacks or allocation.
		for (const auto &delta : proposal.inputs) {
			const CargoType cargo = GetCargoTypeByLabel(delta.label);
			std::ranges::find(this->industry.accepted, cargo, &Industry::AcceptedCargo::cargo)->waiting -= static_cast<uint16_t>(delta.amount);
			this->state.consumed.at(delta.label) += delta.amount;
		}
		for (const auto &delta : proposal.outputs) {
			const CargoType cargo = GetCargoTypeByLabel(delta.label);
			auto slot = std::ranges::find(this->industry.produced, cargo, &Industry::ProducedCargo::cargo);
			slot->waiting += static_cast<uint16_t>(delta.amount);
			slot->history[THIS_MONTH].production += static_cast<uint16_t>(delta.amount);
			this->state.produced.at(delta.label) += delta.amount;
		}
		this->state.work_ticks -= proposal.work_ticks;
		this->state.batches.at(this->state.recipe) += proposal.plan.batches;
		++this->state.revision;
		this->industry.last_prod_year = TimerGameEconomy::year;
		return true;
	}
};

/** @copydoc AdvanceLayerProcessor */
void AdvanceLayerProcessor(Industry &industry, uint32_t economy_ticks)
{
	const auto found = _layer_processors.find(industry.index);
	if (found == _layer_processors.end()) return;
	LayerProcessorState &state = found->second;
	const auto *record = GetLayerProcessorRecipe(industry);
	// Inactive, malformed or exhausted revision/work records hold their real buffers instead of entering legacy conversion.
	if (record == nullptr || !IsLayerProcessorActive(industry, *record) || state.revision == 0 || state.revision == UINT64_MAX || state.scale_fraction >= 200) return;
	const uint64_t cap = static_cast<uint64_t>(record->recipe.work_economy_ticks) * record->recipe.max_batches;
	if (state.work_ticks > cap) return;
	const uint64_t earned = static_cast<uint64_t>(economy_ticks) * _settings_game.economy.industry_cargo_scale * (EconomyIsInRecession() ? 1 : 2) + state.scale_fraction;
	const uint64_t work = std::min<uint64_t>(cap, state.work_ticks + earned / 200);
	const uint16_t fraction = static_cast<uint16_t>(earned % 200);
	// Work uses deterministic saved rate fractions; a saturated bounded work bank cannot grow into an unlimited future burst.
	if (state.work_ticks != work || state.scale_fraction != fraction) {
		state.work_ticks = work;
		state.scale_fraction = fraction;
		++state.revision;
	}
	NativeLayerProcessorInventory inventory(industry, state, *record);
	const auto proposal = PrepareLayerRecipeBatch(record->recipe, inventory);
	CommitLayerRecipeBatch(record->recipe, inventory, inventory, proposal);
}

/** @copydoc SelectLayerProcessorRecipe */
bool SelectLayerProcessorRecipe(Industry &industry, MultiLayerRecipeID recipe, CompanyID company, bool execute)
{
	const auto found = _layer_processors.find(industry.index);
	if (found == _layer_processors.end()) return false;
	LayerProcessorState &state = found->second;
	const auto *record = FindLayerProcessorRecipe(industry.type, recipe);
	if (company != OWNER_DEITY && !Company::IsValidID(company)) return false; // Abandonment never grants recipe control to an invalid actor.
	// A valid company may recover abandoned recipe control; owned control remains exclusive and stocks stay with the facility.
	if (record == nullptr || state.revision == 0 || state.revision == UINT64_MAX ||
			(company != OWNER_DEITY && company != state.controller && state.controller != CompanyID::Invalid()) ||
			TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date).year < record->introduction_year) return false;
	if (execute) {
		if (state.controller == CompanyID::Invalid() && company != OWNER_DEITY) state.controller = company; // Estimates/deity actions never claim control for a nonexistent player company.
		state.recipe = recipe;
		++state.revision;
	}
	return true;
}

/** @copydoc DispatchLayerProcessorCargo */
uint32_t DispatchLayerProcessorCargo(Industry &industry)
{
	const auto found = _layer_processors.find(industry.index);
	if (found == _layer_processors.end()) return 0;
	LayerProcessorState &state = found->second;
	const auto *record = GetLayerProcessorRecipe(industry);
	if (record == nullptr) return 0;
	if (!IsLayerProcessorActive(industry, *record)) return 0;
	uint32_t total = 0;
	// Each product slot commits its own exact packet receipt; a refused/full destination never destroys another product's stock.
	for (auto &output : industry.produced) {
		if (state.revision == UINT64_MAX) break;
		const uint32_t offered = static_cast<uint32_t>(std::min({static_cast<uint64_t>(output.waiting), uint64_t{255},
			UINT64_MAX - state.dispatched, static_cast<uint64_t>(UINT16_MAX - output.history[THIS_MONTH].transported)}));
		if (offered == 0) continue;
		ExactStationSupplyTicket best;
		// Choose actual eligible service with stable identity as a tie-breaker; rating reduces throughput without cargo loss.
		for (const Station *station : industry.stations_near) {
			auto candidate = PrepareExactStationSupply(station->index, output.cargo, offered, {industry.index, SourceType::Industry}, record->layer, industry.exclusive_consumer);
			if (candidate.amount == 0) continue;
			candidate.amount = static_cast<uint16_t>(std::min<uint32_t>(candidate.amount, (offered * (candidate.rating + 1) + 255) / 256));
			if (best.amount == 0 || candidate.rating > best.rating || (candidate.rating == best.rating && candidate.station < best.station)) best = candidate;
		}
		const uint32_t supplied = CommitExactStationSupply(best);
		if (supplied == 0) continue;
		output.waiting -= static_cast<uint16_t>(supplied);
		output.history[THIS_MONTH].transported += static_cast<uint16_t>(supplied);
		state.dispatched += supplied;
		++state.revision;
		total += supplied;
	}
	return total;
}
