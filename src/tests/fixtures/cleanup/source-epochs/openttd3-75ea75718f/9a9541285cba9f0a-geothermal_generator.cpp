/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator.cpp Actual paid native powerhouse construction, physical operation and service; no cargo or utility quantity is duplicated here.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8C/8DC/8E.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "stdafx.h"
#include "geothermal_generator.h"
#include "industry.h"
#include "industrytype.h"
#include "industry_map.h"
#include "company_base.h"
#include "company_func.h"
#include "economy_func.h"
#include "openttd.h"
#include "timer/timer_game_tick.h"
#include "viewport_func.h"
#include "geothermal_collector.h"
#include "geothermal_utility.h"
#include "core/backup_type.hpp"
#include <set>
#include "safeguards.h"

static std::map<IndustryID, GeothermalGeneratorState> _geothermal_generators; ///< Sole powerhouse construction/control records; actual Industry accepted slots retain every material unit.
static std::optional<std::vector<GeothermalGeneratorState>> _pending_geothermal_generators; ///< Detached unpublished file transport; parsing confers no work or stock rights.

/** @copydoc GetGeothermalGeneratorState */
const GeothermalGeneratorState *GetGeothermalGeneratorState(IndustryID industry)
{
	const auto found = _geothermal_generators.find(industry);
	return found == _geothermal_generators.end() ? nullptr : &found->second;
}

/** @copydoc PrepareGeothermalGeneratorConstruction */
GeothermalGeneratorAdmission PrepareGeothermalGeneratorConstruction(GeothermalSiteID site)
{
	GeothermalGeneratorAdmission result; result.site = site;
	const auto *claim = GetGeothermalSiteClaim(site);
	// An original owned active dated generator blueprint is the only construction
	// authority. A collector, retired reservation or foreign site grants none.
	if (claim == nullptr || !claim->active || claim->site.owner != _current_company) return result;
	const auto model = GetMultiLayerGeothermalGeneratorModel(claim->site.kind);
	if (!model.has_value()) return result;
	result.model = *model;
	result.foundation = PrepareGeothermalSite(claim->site);
	if (result.foundation.error != GeothermalError::None) return result;
	result.type = GetGeothermalGeneratorIndustryType(result.model);
	// Preflight the complete bounded native participants before capital or the
	// original planning reservation changes. No utility account is needed to build.
	if (result.type == IT_INVALID || !Industry::CanAllocateItem() || _geothermal_generators.size() >= MAX_GEOTHERMAL_SITE_CLAIMS) {
		result.foundation.error = GeothermalError::Limit; return result;
	}
	if (Money{result.foundation.construction->capital_cost} > GetAvailableMoneyForCommand()) result.foundation.error = GeothermalError::Refused;
	return result;
}

/** @copydoc BindGeothermalGeneratorIndustry */
bool BindGeothermalGeneratorIndustry(Industry &industry, GeothermalSiteID site)
{
	const auto *claim = GetGeothermalSiteClaim(site);
	const auto model = claim == nullptr ? std::nullopt : GetMultiLayerGeothermalGeneratorModel(claim->site.kind);
	// The native pool slot is already reserved. Revalidate all other original
	// participants without falsely requiring a second free slot at the pool limit.
	if (Industry::GetIfValid(industry.index) != &industry || industry.location.w != 0 || _geothermal_generators.contains(industry.index) ||
		claim == nullptr || !claim->active || !model.has_value() || claim->site.owner != _current_company ||
		_geothermal_generators.size() >= MAX_GEOTHERMAL_SITE_CLAIMS || GetGeothermalGeneratorIndustryType(*model) == IT_INVALID) return false;
	const auto admission = PrepareGeothermalSite(claim->site);
	if (admission.error != GeothermalError::None) return false;
	if (Money{admission.construction->capital_cost} > GetAvailableMoneyForCommand()) return false;
	const auto [position, inserted] = _geothermal_generators.emplace(industry.index, GeothermalGeneratorState{industry.index, site, _current_company, *model});
	if (!inserted) return false;
	if (CancelGeothermalSite(site) != GeothermalError::None) { _geothermal_generators.erase(position); return false; }
	return true;
}

/**
 * Validate retained original machinery, material receipt and native presentation independently of temporary owner/date availability.
 * @param state Actual or detached construction record.
 * @param industry Its actual native stock/machinery owner.
 * @return Whether every original floor/cargo/work relationship agrees without repair.
 */
static bool HasGeothermalGeneratorStructure(const GeothermalGeneratorState &state, const Industry &industry)
{
	const auto *operating = GetMultiLayerGeothermalGeneratorTerms(state.model);
	if (operating == nullptr) return false;
	const auto *terms = GetMultiLayerGeothermalConstruction(operating->construction);
	const auto *claim = GetGeothermalSiteClaim(state.site);
	const auto type = GetGeothermalGeneratorIndustryType(state.model);
	// The consumed cold site and the exact native definition establish physical
	// identity. Neither generic completion flags nor copied history can replace them.
	if (terms == nullptr || type == IT_INVALID || claim == nullptr || claim->active || claim->site.kind != terms->kind || state.industry != industry.index ||
		industry.type != type || industry.location.tile != claim->site.tile ||
		industry.location.w != terms->width || industry.location.h != terms->height || industry.accepted.size() != terms->inputs.size() ||
		!industry.produced.empty() || state.work_ticks > terms->work_ticks || state.materials_committed != (state.work_ticks != 0)) return false;
	const auto &site = claim->site;
	// The powerhouse may stand away from its collector, but it must retain an
	// actual Deep site and natural reference. Later physical pipes alone connect heat.
	if (!IsValidTile(site.tile) || GetTileLayer(site.tile) != MapLayer::DeepUnderground ||
		std::ranges::find(GetGeologyWorldState().vents, site.vent, &GeothermalVentField::id) == GetGeologyWorldState().vents.end()) return false;
	const uint8_t stage = static_cast<uint8_t>(std::min<uint32_t>(3, uint32_t{state.work_ticks} * 4 / terms->work_ticks));
	// All sixteen cells must retain their original canonical TIDS, opened floor
	// and the same paid whole-building progress, including unfinished load stages.
	for (const auto &cell : GetIndustrySpec(industry.type)->layouts.front()) {
		const auto tile = TileAddWrap(site.tile, cell.ti.x, cell.ti.y);
		if (!IsValidTile(tile) || !industry.TileBelongsToIndustry(tile) || GetIndustryGfx(tile) != cell.gfx ||
			GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated || GetIndustryConstructionStage(tile) != stage ||
			IsIndustryCompleted(tile) != (state.work_ticks == terms->work_ticks)) return false; // A hole, altered frame or free completion suspends the whole original machine.
	}
	// Each arrived meaning remains in its sole native slot. A committed bill
	// has no remaining stock; excess/wrong labels cannot manufacture a receipt.
	for (const auto &input : terms->inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		const auto slot = industry.GetCargoAccepted(cargo);
		if (slot == industry.accepted.end() || slot->waiting > input.quantity || (state.materials_committed && slot->waiting != 0)) return false;
	}
	return true;
}

/** @param state Original actual controller. @param industry Its native machinery. @return Whether current company/profile/calendar admits prepaid work without changing retained stock/history. */
static bool IsGeothermalGeneratorAvailable(const GeothermalGeneratorState &state, const Industry &industry)
{
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	const auto *operating = GetMultiLayerGeothermalGeneratorTerms(state.model);
	if (operating == nullptr) return false;
	const auto *construction = GetMultiLayerGeothermalConstruction(operating->construction);
	// Technology is calendar policy, not economy-time progress or view choice.
	// Abandonment or a temporary lock suspends work without refunding its history.
	return Company::IsValidID(state.owner) && profile.id == MultiLayerGameplayProfileID::MultilayerIndustry && !profile.legacy &&
		CheckDeepLayerAvailability(profile, TimerGameCalendar::date) && construction != nullptr &&
		TimerGameCalendar::date >= TimerGameCalendar::ConvertYMDToDate(construction->introduction_year, 0, 1) && HasGeothermalGeneratorStructure(state, industry);
}

/** @copydoc GetGeothermalGeneratorInputRoom */
uint32_t GetGeothermalGeneratorInputRoom(const Industry &industry, CargoType cargo)
{
	const auto *state = GetGeothermalGeneratorState(industry.index);
	// Only an actual currently admitted unfinished whole bill can receive stock;
	// completion, an unknown label or lost control never opens a hidden service bin.
	if (state == nullptr || state->materials_committed || !IsValidCargoType(cargo)) return 0;
	if (!IsGeothermalGeneratorAvailable(*state, industry)) return 0;
	const auto *terms = GetMultiLayerGeothermalConstruction(GetMultiLayerGeothermalGeneratorTerms(state->model)->construction);
	const auto input = std::ranges::find(terms->inputs, CargoSpec::Get(cargo)->label, &MultiLayerRecipeCargo::label);
	const auto slot = industry.GetCargoAccepted(cargo);
	if (input == terms->inputs.end() || slot == industry.accepted.end()) return 0; // Generic acceptance cannot convert another cargo into construction supplies.
	const uint32_t accepted = slot->history == nullptr ? 0 : (*slot->history)[THIS_MONTH].accepted;
	return std::min<uint32_t>(input->quantity - slot->waiting, UINT16_MAX - accepted);
}

/** @copydoc ReceiveGeothermalGeneratorCargo */
uint32_t ReceiveGeothermalGeneratorCargo(Industry &industry, CargoType cargo, uint32_t offered)
{
	const auto amount = std::min(offered, GetGeothermalGeneratorInputRoom(industry, cargo));
	if (amount == 0) return 0;
	const auto slot = industry.GetCargoAccepted(cargo);
	auto &history = slot->GetOrCreateHistory(); // Allocate before the first material/history write so refusal cannot half-settle a packet.
	slot->waiting += static_cast<uint16_t>(amount); history[THIS_MONTH].accepted += static_cast<uint16_t>(amount);
	slot->last_accepted = TimerGameEconomy::date; industry.was_cargo_delivered = true;
	return amount;
}

/** @copydoc AdvanceGeothermalGenerator */
void AdvanceGeothermalGenerator(Industry &industry)
{
	const auto found = _geothermal_generators.find(industry.index);
	if (found == _geothermal_generators.end()) return;
	auto &state = found->second;
	// Completed native machinery still needs its four authored status phases
	// repainted. The existing industry dispatcher supplies the actual clock;
	// bounded sixteen-cell dirtiness changes no construction, service, energy,
	// RNG or saved animation ownership, even for a currently unavailable model.
	if (TimerGameTick::counter % 8 == 0) {
		for (const auto tile : industry.location) MarkTileDirtyByTile(tile);
	}
	// Construction is prepaid and needs no electricity from its own unfinished
	// output. Paused/editor/duplicate/unavailable ticks cannot bank future labour.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || !IsGeothermalGeneratorAvailable(state, industry) ||
		(state.has_work_tick && state.last_work_tick >= TimerGameTick::counter)) return;
	const auto *terms = GetMultiLayerGeothermalConstruction(GetMultiLayerGeothermalGeneratorTerms(state.model)->construction);
	if (state.work_ticks == terms->work_ticks) return;
	if (!state.materials_committed) {
		// Require every whole input before removing any actual native units. A
		// late Battery delivery retains the already arrived Steel/metal/electronics.
		for (const auto &input : terms->inputs) {
			if (industry.GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting != input.quantity) return;
		}
		for (const auto &input : terms->inputs) industry.GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting -= static_cast<uint16_t>(input.quantity);
		state.materials_committed = true;
	}
	state.has_work_tick = true; state.last_work_tick = TimerGameTick::counter; ++state.work_ticks;
	const uint8_t stage = static_cast<uint8_t>(std::min<uint32_t>(3, uint32_t{state.work_ticks} * 4 / terms->work_ticks));
	// One native work receipt controls all sixteen machinery sections. Ordinary
	// tile loops never complete a free cell or publish electrical generation.
	for (const auto &cell : GetIndustrySpec(industry.type)->layouts.front()) {
		const auto tile = TileAddWrap(industry.location.tile, cell.ti.x, cell.ti.y);
		SetIndustryConstructionStage(tile, stage);
		if (state.work_ticks == terms->work_ticks) SetIndustryCompleted(tile);
		MarkTileDirtyByTile(tile);
	}
}

/** @copydoc IsGeothermalGeneratorConstructionComplete */
bool IsGeothermalGeneratorConstructionComplete(IndustryID industry)
{
	const auto *state = GetGeothermalGeneratorState(industry);
	const auto *native = Industry::GetIfValid(industry);
	if (state == nullptr || native == nullptr) return false; // A missing/reused native identity cannot impersonate a completed paid machine.
	const auto *operating = GetMultiLayerGeothermalGeneratorTerms(state->model);
	if (operating == nullptr) return false;
	// Readiness grants only subsequent operating admission. It is not a Heat
	// request, electrical publication or permission to skip physical utility paths.
	return _game_mode == GameMode::Normal && state->materials_committed &&
		state->work_ticks == GetMultiLayerGeothermalConstruction(operating->construction)->work_ticks && IsGeothermalGeneratorAvailable(*state, *native);
}

/** @copydoc GetGeothermalGeneratorPort */
TileIndex GetGeothermalGeneratorPort(IndustryID industry)
{
	const auto *state = GetGeothermalGeneratorState(industry);
	const auto *native = Industry::GetIfValid(industry);
	if (state == nullptr || native == nullptr) return INVALID_TILE; // A native pool-slot replacement cannot inherit the original boundary port.
	const auto *claim = GetGeothermalSiteClaim(state->site);
	// Only the funded original consumed blueprint declares a port. It remains
	// installable during construction, but supplies no operating readiness.
	if (claim == nullptr || claim->active || !IsGeothermalGeneratorDefinition(native->type) || native->location.tile != claim->site.tile) return INVALID_TILE;
	return claim->site.endpoint;
}

/** @copydoc HasGeothermalGeneratorInstalledPorts */
bool HasGeothermalGeneratorInstalledPorts(IndustryID industry)
{
	const auto *state = GetGeothermalGeneratorState(industry);
	if (state == nullptr) return false;
	const auto port = GetGeothermalGeneratorPort(industry);
	// Separate purchased cells must exist in both domains at this declared
	// port. A nearby cable, pipe or different controller grants no connection.
	for (const auto domain : {UtilityEnergyDomain::Heat, UtilityEnergyDomain::Electricity}) {
		const auto link = GetGeothermalUtilityLink(port, domain);
		if (!link.has_value() || link->owner != state->owner) return false;
	}
	return true;
}

/** @copydoc ValidateGeothermalGeneratorOperatingReferences */
bool ValidateGeothermalGeneratorOperatingReferences(const GeothermalGeneratorState &state, const UtilityPowerState &utility)
{
	const auto *terms = GetMultiLayerGeothermalGeneratorTerms(state.model);
	if (terms == nullptr) return false;
	// Billing history is duration, not an energy store. Its bounded numerator
	// must be exactly the fraction earned by actual committed service time.
	if (state.service_remainder >= 74 || state.service_remainder != (state.active_service_ticks % 74) * terms->service_cost_per_day % 74 ||
		state.has_operating_tick != (state.active_service_ticks != 0) || (!state.has_operating_tick && state.last_operating_tick != 0)) return false;
	if (state.heat_consumer.base() == UINT32_MAX) {
		return state.electrical_producer.base() == UINT32_MAX && state.active_service_ticks == 0;
	}
	// Issued IDs are dense retained identities in the shared candidate. Resolve
	// only that candidate, never the running world's energy during preview.
	if (state.heat_consumer.base() >= utility.consumers.size()) return false;
	const auto &input = utility.consumers[state.heat_consumer.base()];
	const auto capacity = IntegrateUtilityGeneration(terms->thermal_capacity_w, MAX_UTILITY_INTERVAL_TICKS, 0);
	if (!capacity.has_value() || state.work_ticks != GetMultiLayerGeothermalConstruction(terms->construction)->work_ticks ||
		input.meter.consumer_id != state.heat_consumer || input.effect != UtilityConsumerEffect::HeatGenerator ||
		input.meter.domain != UtilityEnergyDomain::Heat || input.meter.layer != MapLayer::DeepUnderground || input.capacity_j != capacity->energy_j ||
		(input.active && input.owner != state.owner)) return false;
	if (state.electrical_producer.base() == UINT32_MAX) return state.active_service_ticks == 0;
	if (state.electrical_producer.base() >= utility.producers.size()) return false;
	const auto &output = utility.producers[state.electrical_producer.base()];
	// The facility references the one immutable model-specific conversion
	// source. It cannot adopt a numerical generic source or another plant's carry.
	return output.meter.producer_id == state.electrical_producer && output.kind == UtilityGenerationKind::HeatConversion &&
		output.conversion_input == state.heat_consumer && output.conversion_efficiency == terms->efficiency_percent &&
		output.conversion_capacity_w == terms->electrical_capacity_w && (!output.active || output.owner == state.owner) &&
		(output.conversion_origins.empty() == (state.active_service_ticks == 0));
}

/** @param state Actual original facility. @param ticks Actual synchronized duration. @return Whole service invoice and its retained fraction; admission separately reserves enough cash for the fraction too. */
static std::pair<uint64_t, uint8_t> GeothermalGeneratorService(const GeothermalGeneratorState &state, uint32_t ticks)
{
	const uint64_t numerator = uint64_t{GetMultiLayerGeothermalGeneratorTerms(state.model)->service_cost_per_day} * ticks + state.service_remainder;
	return {numerator / 74, static_cast<uint8_t>(numerator % 74)};
}

/** @copydoc HasGeothermalGeneratorServiceFunds */
bool HasGeothermalGeneratorServiceFunds(IndustryID industry, uint32_t economy_ticks)
{
	const auto *state = GetGeothermalGeneratorState(industry);
	// A missing/abandoned owner or invalid duration cannot reserve even the
	// fractional service obligation; the query acquires no operating right.
	if (state == nullptr || !Company::IsValidID(state->owner) || economy_ticks == 0 || economy_ticks > MAX_UTILITY_INTERVAL_TICKS) return false;
	const auto [cost, remainder] = GeothermalGeneratorService(*state, economy_ticks);
	return Company::Get(state->owner)->money >= Money{static_cast<int64_t>(cost + uint64_t{remainder != 0})};
}

/** @param industry Actual completed original machine. @param ticks Actual native duration. @return Whether model, installed ports, output opportunity, duplicate guard and complete service invoice admit work without mutation. */
static bool CanOperateGeothermalGenerator(IndustryID industry, uint32_t ticks)
{
	const auto *state = GetGeothermalGeneratorState(industry);
	// Admission belongs to actual synchronized gameplay, independent of camera.
	// Unavailable construction, pause, wrong actor or duplicate time earns none.
	if (state == nullptr || ticks == 0 || ticks > MAX_UTILITY_INTERVAL_TICKS || _pause_mode.Any() || state->owner != _current_company ||
		!IsGeothermalGeneratorConstructionComplete(industry) || !HasGeothermalGeneratorInstalledPorts(industry) ||
		(state->has_operating_tick && state->last_operating_tick >= TimerGameTick::counter) || ticks > UINT64_MAX - state->active_service_ticks) return false;
	const auto *input = GetUtilityPowerConsumer(state->heat_consumer);
	const auto *output = GetUtilityPowerProducer(state->electrical_producer);
	// Output space is this still-open finite shared publication opportunity,
	// not a private accumulating store. Previous electrical allocation or an
	// existing publication closes it before scarce Heat can be requested.
	if (input == nullptr || output == nullptr || !input->active || !output->active || input->owner != state->owner || output->owner != state->owner ||
		output->conversion_input != state->heat_consumer || IsUtilityPowerAllocated() || output->meter.accounting_interval == GetUtilityPowerInterval()) return false;
	return HasGeothermalGeneratorServiceFunds(industry, ticks);
}

/** @copydoc PrepareGeothermalGeneratorDemand */
std::optional<GeothermalHeatRequest> PrepareGeothermalGeneratorDemand(IndustryID industry, uint32_t economy_ticks)
{
	if (!CanOperateGeothermalGenerator(industry, economy_ticks)) return std::nullopt;
	const auto &state = *GetGeothermalGeneratorState(industry);
	const auto *terms = GetMultiLayerGeothermalGeneratorTerms(state.model);
	const auto bound = IntegrateUtilityGeneration(terms->thermal_capacity_w, economy_ticks, 0);
	if (!bound.has_value()) return std::nullopt;
	const auto port = GetGeothermalGeneratorPort(industry);
	uint64_t available = 0;
	// Quote only actual ready owned original collectors with a published offer
	// and a complete installed cardinal route. Allocation later resolves shared
	// pipe contention; these transient sums confer no reservation or identity.
	for (const auto &collector : CaptureGeothermalCollectors()) {
		if (collector.owner != state.owner || !IsGeothermalCollectorOperational(collector.industry)) continue;
		const auto *source = GetUtilityPowerProducer(collector.heat_producer);
		if (source == nullptr || !source->active || source->meter.accounting_interval != GetUtilityPowerInterval() || source->meter.economy_ticks != economy_ticks) continue;
		const auto path = FindGeothermalUtilityPath(state.owner, UtilityEnergyDomain::Heat, GetGeothermalSiteClaim(collector.site)->site.endpoint, port);
		if (!path.has_value()) continue;
		const auto limit = IntegrateUtilityGeneration(path->capacity_w, economy_ticks, 0);
		if (!limit.has_value()) continue;
		available += std::min({bound->energy_j - available, source->meter.generation_energy_j, limit->energy_j});
		if (available == bound->energy_j) break;
	}
	if (available == 0) return std::nullopt;
	return GeothermalHeatRequest{state.owner, state.heat_consumer, port, UtilityHeatUse::ElectricalGenerator, available};
}

/** Whole actual powerhouse service participant; shared Heat/Electricity/carry/history remain exclusively with the native utility owner. */
class GeothermalGeneratorParticipant final : public GeothermalHeatParticipant {
	IndustryID industry; ///< Original actual native machinery identity.
	uint32_t ticks; ///< Exact published common duration, never animation time.
	mutable std::optional<GeothermalGeneratorState> validated; ///< Complete prevalidated native receipt, acquired before the non-failing commit.
	mutable uint64_t validated_tick = 0; ///< Actual synchronized opportunity observed by complete validation.
	mutable UtilityGrantID validated_grant{}; ///< Exact shared reservation approved by validation.
	mutable uint64_t validated_quantity = 0; ///< Exact original Heat approved by validation, never a live balance.
public:
	/** @param industry Actual original machine. @param ticks Exact common opportunity duration. */
	GeothermalGeneratorParticipant(IndustryID industry, uint32_t ticks) : industry(industry), ticks(ticks) {}
	/** @return Original native machinery identity, for the driver's complete public conversion facade. */
	IndustryID IndustryIdentity() const { return this->industry; }
	/** @return Frozen electrical-generator thermal benefit. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return UtilityHeatUse::ElectricalGenerator; }
	/** @param request Exact physical bounded demand. @return Whether the actual native invoice/output/model/ports still admit it without acquiring identities. */
	bool ValidateDemand(const GeothermalHeatRequest &request) const override
	{
		const auto admitted = PrepareGeothermalGeneratorDemand(this->industry, this->ticks);
		// A caller cannot raise the prepared finite request or substitute another
		// original account/port under this actual machinery's service participant.
		return admitted.has_value() && request.owner == admitted->owner && request.consumer == admitted->consumer && request.endpoint == admitted->endpoint &&
			request.use == admitted->use && request.needed_j > 0 && request.needed_j <= admitted->needed_j;
	}
	/** @param proposal Exact original shared Heat debit. @return Complete native service and actual publication-duration admission, with routed geometry checked by the facade. */
	bool Validate(const UtilityMutationProposal &proposal) const override
	{
		this->validated.reset();
		const auto *state = GetGeothermalGeneratorState(this->industry);
		if (state == nullptr || !CanOperateGeothermalGenerator(this->industry, this->ticks)) return false; // Destroyed/reused machinery or a changed invoice cannot receive a partial benefit.
		const auto demand = PrepareGeothermalGeneratorDemand(this->industry, this->ticks);
		if (!demand.has_value() || proposal.grant.consumer != state->heat_consumer || proposal.quantity_j == 0 || proposal.quantity_j > demand->needed_j) return false; // Only this machine's exact positive feasible debit can authorize service.
		this->validated = *state; this->validated_tick = TimerGameTick::counter;
		this->validated_grant = proposal.grant; this->validated_quantity = proposal.quantity_j;
		return true;
	}
	/** @param proposal Exact revalidated original shared debit. @return Complete native invoice/time receipt or unchanged refusal; no utility mutation occurs inside this callback. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override
	{
		const auto found = _geothermal_generators.find(this->industry);
		// All fallible physical/path preparation completed in Validate. The
		// callback only compares its exact native receipt before a whole invoice.
		if (!this->validated.has_value() || found == _geothermal_generators.end() || found->second != *this->validated ||
			this->validated_tick != TimerGameTick::counter || proposal.grant.consumer != this->validated_grant.consumer ||
			proposal.grant.interval != this->validated_grant.interval || proposal.quantity_j != this->validated_quantity) return false;
		auto &state = found->second;
		const auto [cost, remainder] = GeothermalGeneratorService(state, this->ticks);
		SubtractMoneyFromCompany(state.owner, CommandCost(ExpensesType::Other, Money{static_cast<int64_t>(cost)}));
		state.service_remainder = remainder; state.active_service_ticks += this->ticks;
		state.has_operating_tick = true; state.last_operating_tick = TimerGameTick::counter;
		return true;
	}
};

/** @copydoc ConvertGeothermalGeneratorHeat */
UtilityHeatConversionReceipt ConvertGeothermalGeneratorHeat(IndustryID industry, uint64_t heat_j, uint32_t economy_ticks)
{
	const auto *state = GetGeothermalGeneratorState(industry);
	if (state == nullptr) return {};
	const auto proposal = PrepareUtilityHeatConversion({{state->heat_consumer, GetUtilityPowerInterval()}, state->electrical_producer, heat_j, economy_ticks});
	GeothermalGeneratorParticipant participant(industry, economy_ticks);
	return ConvertGeothermalHeat(proposal, participant);
}

/** @copydoc HasGeothermalGenerators */
bool HasGeothermalGenerators()
{
	// Unfinished isolated construction does not open utility epochs. Once an
	// operating input is issued, continue closing intervals through temporary
	// source/port/date loss so unused historical promises cannot remain live.
	for (const auto &[id, state] : _geothermal_generators) {
		if (state.heat_consumer.base() != UINT32_MAX) return true;
		if (IsGeothermalGeneratorConstructionComplete(id) && HasGeothermalGeneratorInstalledPorts(id)) return true;
	}
	return false;
}

/** @copydoc ValidateGeothermalGeneratorPhysicalReferences */
bool ValidateGeothermalGeneratorPhysicalReferences()
{
	const auto ports = CaptureGeothermalHeatConsumers();
	// Historical inactive inputs also retain their original explicit port.
	// Temporary pipe loss is legal; substituting another endpoint is not.
	for (const auto &[id, state] : _geothermal_generators) {
		if (state.heat_consumer.base() == UINT32_MAX) continue;
		const auto port = std::ranges::find(ports, state.heat_consumer, &GeothermalHeatConsumerBinding::consumer);
		if (port == ports.end() || port->endpoint != GetGeothermalGeneratorPort(id)) return false;
	}
	return true;
}

/** @copydoc RunGeothermalGenerators */
void RunGeothermalGenerators(uint64_t interval, uint32_t economy_ticks, std::span<const GeothermalHeatDemand> other_demands)
{
	// An unrelated orbital/external electrical world must not acquire a new
	// thermal domain merely because it uses the shared synchronized driver.
	if (!HasUtilityHeatState() && !HasGeothermalGenerators()) return;
	if (other_demands.size() > MAX_UTILITY_INTERVAL_ACCOUNTS) return; // Bound the complete external demand set before acquiring any native generator identity or callback storage.
	// Only the common synchronized owner acquires identities or closes Heat.
	// Preparation, viewport queries and repeated closed passes remain pure.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || interval != GetUtilityPowerInterval() || IsUtilityHeatAllocated() || IsUtilityPowerAllocated() ||
		economy_ticks == 0 || economy_ticks > MAX_UTILITY_INTERVAL_TICKS) return;
	std::vector<GeothermalGeneratorParticipant> participants; participants.reserve(_geothermal_generators.size());
	std::vector<GeothermalHeatDemand> demands(other_demands.begin(), other_demands.end());
	demands.reserve(other_demands.size() + _geothermal_generators.size());
	// Native IndustryID order acquires permanent shared identities once. A
	// capacity-limited output registration retains its issued input for retry;
	// it never requests Heat until its complete original pair exists.
	for (auto &[id, state] : _geothermal_generators) {
		if (!IsGeothermalGeneratorConstructionComplete(id) || !HasGeothermalGeneratorInstalledPorts(id)) continue;
		AutoRestoreBackup actor(_current_company, state.owner);
		const auto *terms = GetMultiLayerGeothermalGeneratorTerms(state.model);
		if (state.heat_consumer.base() == UINT32_MAX) {
			const auto capacity = IntegrateUtilityGeneration(terms->thermal_capacity_w, MAX_UTILITY_INTERVAL_TICKS, 0);
			if (!capacity.has_value()) continue;
			const auto input = RegisterGeothermalHeatConsumer(state.owner, GetGeothermalGeneratorPort(id), UtilityHeatUse::ElectricalGenerator, capacity->energy_j);
			if (!input.has_value()) continue;
			state.heat_consumer = *input;
		}
		if (state.electrical_producer.base() == UINT32_MAX) {
			const auto output = RegisterUtilityHeatConversionProducer(state.owner, state.heat_consumer, terms->efficiency_percent, terms->electrical_capacity_w);
			if (!output.has_value()) continue;
			state.electrical_producer = *output;
		}
		const auto request = PrepareGeothermalGeneratorDemand(id, economy_ticks);
		if (!request.has_value()) continue;
		participants.emplace_back(id, economy_ticks); demands.push_back({*request, &participants.back()});
	}
	AutoRestoreBackup system(_current_company, OWNER_DEITY);
	if (AllocateVentHeat(interval, demands) != UtilityPowerError::None) return;
	// Shared ConsumerID order settles contention across both native and existing
	// physical consumers. Only generators commit here; other complete owners
	// retain their actual consume/benefit boundary and cannot be impersonated.
	for (const auto &participant : participants) {
		const auto *state = GetGeothermalGeneratorState(participant.IndustryIdentity());
		if (state == nullptr) continue;
		const auto *consumer = GetUtilityPowerConsumer(state->heat_consumer);
		if (consumer == nullptr) continue;
		uint64_t remaining = 0;
		for (const auto &share : consumer->shares) remaining += share.allocated_j - share.consumed_j - share.released_j - share.transferred_j;
		if (remaining == 0) continue;
		AutoRestoreBackup actor(_current_company, state->owner);
		ConvertGeothermalGeneratorHeat(state->industry, remaining, economy_ticks);
	}
}

/** @copydoc GeothermalGeneratorHasStoredCargo */
bool GeothermalGeneratorHasStoredCargo(const Industry &industry)
{
	if (GetGeothermalGeneratorState(industry.index) == nullptr && !IsGeothermalGeneratorDefinition(industry.type)) return false; // Staged machinery also protects its real stock before binding publication.
	/** @param slot Existing native input or output inventory. @return Whether demolition would discard retained physical material. */
	const auto has_stock = [](const auto &slot) { return slot.waiting != 0; };
	return std::ranges::any_of(industry.accepted, has_stock) || std::ranges::any_of(industry.produced, has_stock);
}

/** @copydoc ReleaseGeothermalGeneratorIndustry */
void ReleaseGeothermalGeneratorIndustry(IndustryID industry)
{
	const auto found = _geothermal_generators.find(industry);
	if (found == _geothermal_generators.end()) return;
	if (found->second.heat_consumer.base() != UINT32_MAX) {
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		RetireUtilityHeatConversion(found->second.heat_consumer, found->second.electrical_producer);
	}
	_geothermal_generators.erase(found);
}

/** @copydoc ChangeGeothermalGeneratorOwnership */
void ChangeGeothermalGeneratorOwnership(Owner old_owner, Owner new_owner)
{
	// Takeover transfers actual control; loss retains the paid machinery/work
	// as abandoned. Neither event can refund capital or recreate consumed stock.
	for (auto &[id, state] : _geothermal_generators) {
		if (state.owner == old_owner) state.owner = Company::IsValidID(new_owner) ? CompanyID{new_owner} : CompanyID::Invalid();
	}
}

/** @copydoc CaptureGeothermalGenerators */
std::vector<GeothermalGeneratorState> CaptureGeothermalGenerators()
{
	std::vector<GeothermalGeneratorState> result; result.reserve(_geothermal_generators.size());
	for (const auto &[id, state] : _geothermal_generators) result.push_back(state); // Stable actual native ID order, never another inventory.
	return result;
}

/** @copydoc ValidateGeothermalGenerators */
bool ValidateGeothermalGenerators(std::span<const GeothermalGeneratorState> states)
{
	if (states.size() > MAX_GEOTHERMAL_SITE_CLAIMS) return false;
	std::set<IndustryID> industries; std::set<GeothermalSiteID> sites;
	std::set<UtilityConsumerID> inputs; std::set<UtilityProducerID> outputs;
	const auto utility = CaptureUtilityPowerState();
	// Every complete candidate must retain unique original work/site identities,
	// exact native stock/tiles and an actual surviving or explicitly lost owner.
	for (const auto &state : states) {
		const auto *industry = Industry::GetIfValid(state.industry);
		if (industry == nullptr || !industries.insert(state.industry).second || !sites.insert(state.site).second ||
			(state.owner != CompanyID::Invalid() && !Company::IsValidID(state.owner)) ||
			state.has_work_tick != (state.work_ticks != 0) || (!state.has_work_tick && state.last_work_tick != 0) ||
			(state.has_work_tick && state.last_work_tick > TimerGameTick::counter) || !HasGeothermalGeneratorStructure(state, *industry) ||
			(state.has_operating_tick && state.last_operating_tick > TimerGameTick::counter) || !ValidateGeothermalGeneratorOperatingReferences(state, utility) ||
			(state.heat_consumer.base() != UINT32_MAX && !inputs.insert(state.heat_consumer).second) ||
			(state.electrical_producer.base() != UINT32_MAX && !outputs.insert(state.electrical_producer).second)) return false;
	}
	// Exact native-family coverage rejects an omitted table/row instead of
	// inferring free construction from a saved industry definition or completed bit.
	for (const Industry *industry : Industry::Iterate()) {
		if (IsGeothermalGeneratorDefinition(industry->type) && !industries.contains(industry->index)) return false;
	}
	return true;
}

/** @copydoc StageLoadedGeothermalGenerators */
void StageLoadedGeothermalGenerators(std::vector<GeothermalGeneratorState> states) { _pending_geothermal_generators = std::move(states); }

/** @copydoc AcceptLoadedGeothermalGenerators */
bool AcceptLoadedGeothermalGenerators()
{
	const auto states = _pending_geothermal_generators.has_value() ? std::span<const GeothermalGeneratorState>{*_pending_geothermal_generators} : std::span<const GeothermalGeneratorState>{};
	if (!ValidateGeothermalGenerators(states)) return false;
	std::map<IndustryID, GeothermalGeneratorState> accepted;
	for (const auto &state : states) accepted.emplace(state.industry, state); // Acquire the whole validated candidate before publishing any work/control rights.
	_geothermal_generators = std::move(accepted); _pending_geothermal_generators.reset(); return true;
}

/** @copydoc ResetGeothermalGenerators */
void ResetGeothermalGenerators() { _geothermal_generators.clear(); _pending_geothermal_generators.reset(); ResetGeothermalGeneratorLoadStaging(); }

/** @copydoc ComputeGeothermalGeneratorDigest */
uint64_t ComputeGeothermalGeneratorDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Explicit authoritative scalar; fixed byte order excludes padding, pointers and presentation caches. */
	const auto field = [&hash](uint64_t value) {
		for (uint byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; }
	};
	field(_geothermal_generators.size());
	// Hash actual work and borrowed native stock once in canonical identity
	// order. This query cannot publish energy, mutate cargo or advance time.
	for (const auto &[id, state] : _geothermal_generators) {
		field(id.base()); field(state.site.base()); field(state.owner.base()); field(to_underlying(state.model)); field(state.work_ticks);
		field(state.materials_committed); field(state.has_work_tick); field(state.last_work_tick);
		if (state.heat_consumer.base() != UINT32_MAX) {
			// Earlier unbound427/428 native digests retain their exact field order.
			// New operating references/service enter this owner only once issued.
			field(state.heat_consumer.base()); field(state.electrical_producer.base()); field(state.service_remainder);
			field(state.has_operating_tick); field(state.last_operating_tick); field(state.active_service_ticks);
		}
		const auto *industry = Industry::GetIfValid(id);
		if (industry == nullptr) { field(UINT64_MAX); continue; }
		for (const auto &slot : industry->accepted) { field(slot.cargo); field(slot.waiting); field(slot.last_accepted.base()); }
	}
	return hash;
}
