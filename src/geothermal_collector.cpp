/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_collector.cpp Paid native collector construction, one real material receipt and exact actual-source readiness.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1B
 */

#include "stdafx.h"
#include "geothermal_collector.h"
#include "industry.h"
#include "industrytype.h"
#include "company_base.h"
#include "company_func.h"
#include "economy_func.h"
#include "openttd.h"
#include "timer/timer_game_tick.h"
#include "core/backup_type.hpp"
#include "viewport_func.h"
#include <set>
#include "safeguards.h"

static std::map<IndustryID, GeothermalCollectorState> _geothermal_collectors; ///< Sole actual construction/control bindings; Industry retains all input/history and geology all natural capacity.
static std::optional<std::vector<GeothermalCollectorState>> _pending_geothermal_collectors; ///< Unaccepted parsed construction; no native stock/work/source authority is published during file parsing.

/** @copydoc GetGeothermalCollectorState */
const GeothermalCollectorState *GetGeothermalCollectorState(IndustryID industry)
{
	const auto found = _geothermal_collectors.find(industry);
	return found == _geothermal_collectors.end() ? nullptr : &found->second;
}

/** @copydoc FindGeothermalCollectorForVent */
IndustryID FindGeothermalCollectorForVent(GeothermalVentID vent)
{
	// Bounded actual machinery bindings preserve source exclusivity even when
	// construction waits or its company disappears; a second building must not
	// publish another private copy of the same natural field's capacity.
	for (const auto &[id, state] : _geothermal_collectors) {
		const auto *claim = GetGeothermalSiteClaim(state.site);
		if (claim != nullptr && claim->site.vent == vent) return id; // Only the original retained blueprint establishes this physical source association.
	}
	return IndustryID::Invalid();
}

/** @copydoc PrepareGeothermalCollectorConstruction */
GeothermalCollectorAdmission PrepareGeothermalCollectorConstruction(GeothermalSiteID site)
{
	GeothermalCollectorAdmission result; result.site = site;
	const auto *claim = GetGeothermalSiteClaim(site);
	// Only a current collector reservation can become physical machinery.
	// Cancelled/history, foreign and powerhouse blueprints cannot bypass funding.
	if (claim == nullptr || !claim->active || claim->site.kind != MultiLayerGeothermalConstructionKind::Collector || claim->site.owner != _current_company) return result;
	result.foundation = PrepareGeothermalSite(claim->site);
	if (result.foundation.error != GeothermalError::None) return result;
	result.type = GetGeothermalCollectorIndustryType();
	// Both native pool and bounded binding must fit before capital or the cold
	// reservation changes. Existing unfinished/abandoned collectors remain the
	// exclusive source participant until actual safe demolition.
	if (result.type == IT_INVALID || !Industry::CanAllocateItem() || _geothermal_collectors.size() >= MAX_GEOTHERMAL_SITE_CLAIMS ||
		FindGeothermalCollectorForVent(claim->site.vent) != IndustryID::Invalid()) { result.foundation.error = GeothermalError::Limit; return result; }
	if (Money{result.foundation.construction->capital_cost} > GetAvailableMoneyForCommand()) result.foundation.error = GeothermalError::Refused;
	return result;
}

/** @copydoc BindGeothermalCollectorIndustry */
bool BindGeothermalCollectorIndustry(Industry &industry, GeothermalSiteID site)
{
	// A populated industry cannot be rebound over another stock/floor owner.
	// Allocation precedes the cold retirement, leaving refusal wholly unchanged.
	if (Industry::GetIfValid(industry.index) != &industry || industry.location.w != 0 || _geothermal_collectors.contains(industry.index)) return false;
	const auto *claim = GetGeothermalSiteClaim(site);
	// The native pool slot is already reserved here. Revalidate the remaining
	// participants without requiring a second free slot at the pool limit.
	if (claim == nullptr || !claim->active || claim->site.owner != _current_company || claim->site.kind != MultiLayerGeothermalConstructionKind::Collector ||
		_geothermal_collectors.size() >= MAX_GEOTHERMAL_SITE_CLAIMS || GetGeothermalCollectorIndustryType() == IT_INVALID ||
		FindGeothermalCollectorForVent(claim->site.vent) != IndustryID::Invalid()) return false;
	const auto admission = PrepareGeothermalSite(claim->site);
	if (admission.error != GeothermalError::None) return false;
	if (Money{admission.construction->capital_cost} > GetAvailableMoneyForCommand()) return false;
	const auto [position, inserted] = _geothermal_collectors.emplace(industry.index, GeothermalCollectorState{industry.index, site, _current_company});
	if (!inserted) return false;
	if (CancelGeothermalSite(site) != GeothermalError::None) { _geothermal_collectors.erase(position); return false; }
	return true;
}

/** @param state Actual construction/control record. @param industry Actual native inventory/machinery. @return Whether the original source, exact complete native floor, material receipt and work agree, independently of temporary owner/date/pause availability. */
static bool HasGeothermalCollectorStructure(const GeothermalCollectorState &state, const Industry &industry)
{
	const auto *claim = GetGeothermalSiteClaim(state.site);
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector);
	// Consumed blueprints retain geometry, while completed flags must reflect
	// paid work/materials rather than generic tile loops or fabricated save flags.
	if (state.industry != industry.index || claim == nullptr || claim->active || claim->site.kind != terms->kind || !IsGeothermalCollectorDefinition(industry.type) ||
		state.work_ticks > terms->work_ticks || state.materials_committed != (state.work_ticks != 0) || industry.location.tile != claim->site.tile ||
		industry.location.w != terms->width || industry.location.h != terms->height || industry.accepted.size() != terms->inputs.size() || !industry.produced.empty()) return false;
	const auto &site = claim->site;
	if (!IsValidTile(site.tile) || GetTileLayer(site.tile) != MapLayer::DeepUnderground) return false; // Saved/runtime machinery cannot relocate onto a coincident Surface source.
	const auto &vents = GetGeologyWorldState().vents;
	const auto vent = std::ranges::find(vents, site.vent, &GeothermalVentField::id);
	if (vent == vents.end()) return false;
	// Whole original floor must contact its actual renewable field; a retained
	// cancelled planning ID alone cannot bind a remote natural source.
	if (TileX(site.tile) >= static_cast<uint32_t>(vent->x) + vent->width || TilePlaneY(site.tile) >= static_cast<uint32_t>(vent->y) + vent->height ||
		TileX(site.tile) + terms->width <= vent->x || TilePlaneY(site.tile) + terms->height <= vent->y) return false;
	const auto *spec = GetIndustrySpec(industry.type);
	// Whole native machinery must match the original exact layout and one paid
	// completion boundary; an empty or freely completed cell suspends readiness.
	for (const auto &cell : spec->layouts.front()) {
		const auto tile = TileAddWrap(site.tile, cell.ti.x, cell.ti.y);
		if (!IsValidTile(tile) || !industry.TileBelongsToIndustry(tile) || GetIndustryGfx(tile) != cell.gfx ||
			GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated || IsIndustryCompleted(tile) != (state.work_ticks == terms->work_ticks)) return false; // Every physical cell must match the same paid construction state; holes or free completion suspend the whole collector.
	}
	// A consumed bill leaves no hidden/excess stock. Before consumption each
	// actual native slot admits only its own remaining frozen bill, exactly once.
	for (const auto &input : terms->inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		const auto slot = industry.GetCargoAccepted(cargo);
		if (slot == industry.accepted.end() || slot->waiting > input.quantity || (state.materials_committed && slot->waiting != 0)) return false; // Wrong meanings/overdelivery cannot manufacture a receipt or discard excess input.
	}
	return true;
}

/** @param state Actual controller/construction state. @param industry Real native machinery. @return Whether the actual shared profile/company/calendar admits construction or source operation now; temporary refusal never erases paid work or stock. */
static bool IsGeothermalCollectorAvailable(const GeothermalCollectorState &state, const Industry &industry)
{
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	return Company::IsValidID(state.owner) && profile.id == MultiLayerGameplayProfileID::MultilayerIndustry && !profile.legacy &&
		CheckDeepLayerAvailability(profile, TimerGameCalendar::date) && HasGeothermalCollectorStructure(state, industry);
}

/** @copydoc GetGeothermalCollectorInputRoom */
uint32_t GetGeothermalCollectorInputRoom(const Industry &industry, CargoType cargo)
{
	const auto *state = GetGeothermalCollectorState(industry.index);
	if (state == nullptr || state->materials_committed || !IsValidCargoType(cargo)) return 0; // Only an actual unfinished bill can receive construction supplies.
	if (!IsGeothermalCollectorAvailable(*state, industry)) return 0;
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector);
	const auto input = std::ranges::find(terms->inputs, CargoSpec::Get(cargo)->label, &MultiLayerRecipeCargo::label);
	const auto slot = industry.GetCargoAccepted(cargo);
	if (input == terms->inputs.end() || slot == industry.accepted.end()) return 0; // Generic acceptance cannot turn another cargo into construction supplies.
	const auto accepted = slot->history == nullptr ? 0 : (*slot->history)[THIS_MONTH].accepted;
	return std::min<uint32_t>(input->quantity - slot->waiting, UINT16_MAX - accepted);
}

/** @copydoc ReceiveGeothermalCollectorCargo */
uint32_t ReceiveGeothermalCollectorCargo(Industry &industry, CargoType cargo, uint32_t offered)
{
	const auto amount = std::min(offered, GetGeothermalCollectorInputRoom(industry, cargo));
	if (amount == 0) return 0;
	const auto slot = industry.GetCargoAccepted(cargo);
	auto &history = slot->GetOrCreateHistory(); // Allocate history before the first exact native stock write.
	slot->waiting += static_cast<uint16_t>(amount);
	history[THIS_MONTH].accepted += static_cast<uint16_t>(amount);
	slot->last_accepted = TimerGameEconomy::date;
	industry.was_cargo_delivered = true;
	return amount;
}

/** @copydoc AdvanceGeothermalCollector */
void AdvanceGeothermalCollector(Industry &industry)
{
	const auto found = _geothermal_collectors.find(industry.index);
	if (found == _geothermal_collectors.end()) return;
	auto &state = found->second;
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector);
	// Native game ticks, valid technology/control and the full real structure
	// earn prepaid labour. Editor/pause, completed work and duplicate tick calls
	// retain the same stock/work and never bank a future catch-up burst.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || !IsGeothermalCollectorAvailable(state, industry) || state.work_ticks == terms->work_ticks ||
		(state.has_work_tick && state.last_work_tick >= TimerGameTick::counter)) return;
	if (!state.materials_committed) {
		// Preflight every actual stock before consuming any input. A missing
		// Electronics delivery must retain all arrived Steel/Refined Metals.
		for (const auto &input : terms->inputs) {
			if (industry.GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting != input.quantity) return;
		}
		for (const auto &input : terms->inputs) industry.GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting -= static_cast<uint16_t>(input.quantity);
		state.materials_committed = true;
	}
	state.has_work_tick = true; state.last_work_tick = TimerGameTick::counter; ++state.work_ticks;
	// All nine cells show the same paid whole-facility progress. Native tile
	// completion is committed only at296 ticks after the complete material bill.
	for (const auto &cell : GetIndustrySpec(industry.type)->layouts.front()) {
		const auto tile = TileAddWrap(industry.location.tile, cell.ti.x, cell.ti.y);
		SetIndustryConstructionStage(tile, static_cast<uint8_t>(std::min<uint16_t>(3, state.work_ticks / 74)));
		if (state.work_ticks == terms->work_ticks) SetIndustryCompleted(tile);
		MarkTileDirtyByTile(tile);
	}
}

/** @copydoc IsGeothermalCollectorOperational */
bool IsGeothermalCollectorOperational(IndustryID industry)
{
	const auto *state = GetGeothermalCollectorState(industry);
	const auto *native = Industry::GetIfValid(industry);
	if (state == nullptr || native == nullptr) return false; // A restored/deleted native ID cannot act as a stand-in completed collector.
	return _game_mode == GameMode::Normal && state->work_ticks == GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->work_ticks &&
		state->materials_committed && IsGeothermalCollectorAvailable(*state, *native);
}

/** @copydoc GeothermalCollectorHasStoredCargo */
bool GeothermalCollectorHasStoredCargo(const Industry &industry)
{
	if (GetGeothermalCollectorState(industry.index) == nullptr && !IsGeothermalCollectorDefinition(industry.type)) return false; // Restoring machinery also retains stock before its binding publishes.
	return std::ranges::any_of(industry.accepted, [](const auto &slot) { return slot.waiting != 0; }) ||
		std::ranges::any_of(industry.produced, [](const auto &slot) { return slot.waiting != 0; });
}

/** @copydoc PublishGeothermalCollectorHeat */
UtilityPowerError PublishGeothermalCollectorHeat(IndustryID industry, uint64_t interval, uint32_t economy_ticks, bool execute)
{
	const auto found = _geothermal_collectors.find(industry);
	if (found == _geothermal_collectors.end()) return UtilityPowerError::InvalidReference;
	auto &state = found->second;
	if (state.owner != _current_company) return UtilityPowerError::InvalidOwner;
	if (!IsGeothermalCollectorOperational(industry)) return UtilityPowerError::Refused;
	if (_pause_mode.Any()) return UtilityPowerError::Refused;
	// The driver owns the epoch and duration. Reject stale, closed or excessive
	// offers before acquiring a meter, so refusal cannot leave a new identity.
	if (interval != GetUtilityPowerInterval() || interval == UINT64_MAX) return UtilityPowerError::InvalidInterval;
	if (IsUtilityHeatAllocated()) return UtilityPowerError::AlreadyAllocated;
	if (economy_ticks == 0) return UtilityPowerError::InvalidQuantity;
	const auto *claim = GetGeothermalSiteClaim(state.site);
	const auto &vents = GetGeologyWorldState().vents;
	const auto vent = std::ranges::find(vents, claim->site.vent, &GeothermalVentField::id);
	if (vent == vents.end()) return UtilityPowerError::InvalidReference;
	if (!IntegrateUtilityGeneration(vent->thermal_capacity_w, economy_ticks, 0).has_value()) return UtilityPowerError::InvalidQuantity;
	if (state.heat_producer == UtilityProducerID{UINT32_MAX}) {
		const auto candidate = RegisterUtilityHeatProducer(state.owner, false);
		if (!candidate.has_value()) return UtilityPowerError::Limit;
		// Association plus publication need two sequence values. Acquire neither
		// at exhaustion; the accepted native meter will own all energy and carry.
		if (CaptureUtilityPowerState().operation_sequence >= UINT64_MAX - 2) return UtilityPowerError::Limit;
		if (!execute) return UtilityPowerError::None;
		const auto registered = RegisterUtilityHeatProducer(state.owner);
		if (!registered.has_value()) return UtilityPowerError::Refused;
		state.heat_producer = *registered;
	}
	const auto *source = GetUtilityPowerProducer(state.heat_producer);
	// A retired, foreign or electrical meter cannot be rebound to this machinery.
	// Actual demolition retires the meter; reconstruction acquires a fresh ID.
	if (source == nullptr || !source->active || source->owner != state.owner || source->meter.domain != UtilityEnergyDomain::Heat ||
		source->meter.layer != MapLayer::DeepUnderground || source->kind != UtilityGenerationKind::Geothermal) return UtilityPowerError::InvalidReference;
	return PublishUtilityHeatGeneration(state.heat_producer, interval, vent->thermal_capacity_w, economy_ticks, execute);
}

/** @copydoc HasGeothermalHeatSources */
bool HasGeothermalHeatSources()
{
	// Keep closing intervals after a temporary source/date interruption. Without
	// this retained binding, a last unused promise could survive indefinitely.
	for (const auto &[id, state] : _geothermal_collectors) {
		if (state.heat_producer != UtilityProducerID{UINT32_MAX} || IsGeothermalCollectorOperational(id)) return true;
	}
	return false;
}

/** @copydoc PublishGeothermalHeatSources */
void PublishGeothermalHeatSources(uint64_t interval, uint32_t economy_ticks)
{
	// Iterate only bounded actual machinery, in stable native ID order. Every
	// company publishes before the one shared allocation; no map scan is needed.
	for (const auto &[id, state] : _geothermal_collectors) {
		if (!IsGeothermalCollectorOperational(id)) continue;
		AutoRestoreBackup actor(_current_company, state.owner);
		PublishGeothermalCollectorHeat(id, interval, economy_ticks);
	}
}

/** @copydoc ReleaseGeothermalCollectorIndustry */
void ReleaseGeothermalCollectorIndustry(IndustryID industry)
{
	const auto found = _geothermal_collectors.find(industry);
	if (found == _geothermal_collectors.end()) return;
	if (found->second.heat_producer != UtilityProducerID{UINT32_MAX}) {
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		RemoveUtilityPowerProducer(found->second.heat_producer); // Release only unused rights; shared consumed history survives physical removal.
	}
	_geothermal_collectors.erase(found);
}
/** @copydoc ChangeGeothermalCollectorOwnership */
void ChangeGeothermalCollectorOwnership(Owner old_owner, Owner new_owner)
{
	// Ownership follows actual takeover without reconstructing machinery,
	// refunding capital or consuming the construction bill a second time.
	for (auto &[id, state] : _geothermal_collectors) {
		if (state.owner == old_owner) state.owner = Company::IsValidID(new_owner) ? CompanyID{new_owner} : CompanyID::Invalid();
	}
}
/** @copydoc ResetGeothermalCollectors */
void ResetGeothermalCollectors()
{
	_geothermal_collectors.clear(); _pending_geothermal_collectors.reset(); ResetGeothermalCollectorLoadStaging();
}

/** @copydoc CaptureGeothermalCollectors */
std::vector<GeothermalCollectorState> CaptureGeothermalCollectors()
{
	std::vector<GeothermalCollectorState> rows; rows.reserve(_geothermal_collectors.size());
	// Canonical native ID order transports only actual construction/control;
	// the native industry chunk remains the sole material/history transport.
	for (const auto &[id, state] : _geothermal_collectors) rows.push_back(state);
	return rows;
}

/** @copydoc ValidateGeothermalCollectors */
bool ValidateGeothermalCollectors(std::span<const GeothermalCollectorState> rows, bool world_refs)
{
	if (rows.size() > MAX_GEOTHERMAL_SITE_CLAIMS) return false;
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector);
	std::set<IndustryID> industries; std::set<GeothermalSiteID> sites; std::set<GeothermalVentID> sources;
	std::set<UtilityProducerID> meters;
	const auto cold = world_refs ? CaptureGeothermalSiteClaims() : std::vector<GeothermalSiteClaim>{};
	// Bounded actual bindings must preserve one native industry, consumed
	// blueprint and original source. No corrupt row is repaired into machinery,
	// a paid receipt, an owner or additional construction progress.
	for (const auto &state : rows) {
		if (state.industry.base() >= 64000 || state.site.base() >= MAX_GEOTHERMAL_SITE_CLAIMS ||
			(state.owner.base() >= MAX_COMPANIES && state.owner != CompanyID::Invalid()) || state.work_ticks > terms->work_ticks ||
			state.materials_committed != (state.work_ticks != 0) || state.has_work_tick != (state.work_ticks != 0) ||
			(!state.has_work_tick && state.last_work_tick != 0) || !industries.insert(state.industry).second || !sites.insert(state.site).second) return false; // Invalid scalars/duplicate identities cannot mint or lose construction history.
		if (!world_refs) continue;
		const auto *industry = Industry::GetIfValid(state.industry); const auto *claim = GetGeothermalSiteClaim(state.site);
		if (industry == nullptr || claim == nullptr || !HasGeothermalCollectorStructure(state, *industry) ||
			(state.owner != CompanyID::Invalid() && !Company::IsValidID(state.owner)) || !sources.insert(claim->site.vent).second ||
			(state.has_work_tick && state.last_work_tick > TimerGameTick::counter)) return false; // Actual map/company/stock/source and synchronized clock must agree before source readiness publishes.
		if (state.heat_producer != UtilityProducerID{UINT32_MAX}) {
			const auto *meter = GetUtilityPowerProducer(state.heat_producer);
			// Saved associations resolve the same actual Heat meter and original
			// natural rate; an old completed flag cannot authorize invented supply.
			if (state.work_ticks != terms->work_ticks || meter == nullptr || !meters.insert(state.heat_producer).second ||
				meter->meter.domain != UtilityEnergyDomain::Heat || meter->meter.layer != MapLayer::DeepUnderground || meter->kind != UtilityGenerationKind::Geothermal ||
				(meter->active && meter->owner != state.owner)) return false;
			const auto vent = std::ranges::find(GetGeologyWorldState().vents, claim->site.vent, &GeothermalVentField::id);
			if (meter->meter.generation_rate_w != 0 && meter->meter.generation_rate_w != vent->thermal_capacity_w) return false; // A save cannot inflate a collector's sustainable rate beyond its original immutable vent.
		}
		for (const auto &reservation : cold) {
			if (reservation.active && reservation.site.kind == terms->kind && reservation.site.vent == claim->site.vent) return false; // A surviving cold blueprint cannot also reserve an already bound physical collector's source.
		}
	}
	if (!world_refs) return true;
	// Exact native-family coverage catches omitted construction rows even when
	// a malicious file clears its collector feature bit or submits an empty table.
	for (const auto *industry : Industry::Iterate()) {
		if (IsGeothermalCollectorDefinition(industry->type) && !industries.contains(industry->index)) return false; // An actual collector cannot receive implicit free construction on load.
	}
	return true;
}

/** @copydoc StageLoadedGeothermalCollectors */
void StageLoadedGeothermalCollectors(std::vector<GeothermalCollectorState> rows) { _pending_geothermal_collectors = std::move(rows); }
/** @copydoc AcceptLoadedGeothermalCollectors */
bool AcceptLoadedGeothermalCollectors()
{
	const auto rows = _pending_geothermal_collectors.has_value() ? std::span<const GeothermalCollectorState>{*_pending_geothermal_collectors} : std::span<const GeothermalCollectorState>{};
	if (!ValidateGeothermalCollectors(rows)) return false;
	std::map<IndustryID, GeothermalCollectorState> accepted;
	for (const auto &state : rows) accepted.emplace(state.industry, state); // Build the complete validated family before publishing any actual construction/source rights.
	_geothermal_collectors = std::move(accepted); _pending_geothermal_collectors.reset(); return true;
}

/** @copydoc ComputeGeothermalCollectorDigest */
uint64_t ComputeGeothermalCollectorDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Explicit authoritative scalar; fixed bytes exclude host padding, pointers and presentation state. */
	const auto field = [&hash](uint64_t value) {
		for (uint byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; }
	};
	field(_geothermal_collectors.size());
	// Read actual construction and its native inventory once in canonical ID
	// order. This checksum owns neither cargo nor the source's heat capacity.
	for (const auto &[id, state] : _geothermal_collectors) {
		field(id.base()); field(state.site.base()); field(state.owner.base()); field(state.work_ticks);
		field(state.materials_committed); field(state.has_work_tick); field(state.last_work_tick);
		field(state.heat_producer.base());
		const auto *industry = Industry::GetIfValid(id);
		if (industry == nullptr) { field(UINT64_MAX); continue; }
		for (const auto &input : industry->accepted) {
			field(input.cargo); field(input.waiting); field(input.last_accepted.base());
		}
	}
	return hash;
}
