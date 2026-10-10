/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_riser_construction.cpp Paid native riser machinery, delivered construction stock and synchronized labour.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "stdafx.h"
#include "geothermal_riser.h"
#include "geothermal_riser_wire.h"
#include "utility_power.h"
#include "industry.h"
#include "industrytype.h"
#include "company_base.h"
#include "company_func.h"
#include "economy_func.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "openttd.h"
#include "timer/timer_game_tick.h"
#include "viewport_func.h"
#include <set>
#include "safeguards.h"

static std::map<IndustryID, GeothermalRiserState> _geothermal_risers; ///< Bounded machinery associations only; native Industry owns every delivered material/history unit.
static std::optional<std::vector<GeothermalRiserState>> _pending_geothermal_risers; ///< Unpublished parsed references; native quantities remain with their existing owners.

/** @copydoc GetGeothermalRiserState */
const GeothermalRiserState *GetGeothermalRiserState(IndustryID industry)
{
	const auto found = _geothermal_risers.find(industry);
	return found == _geothermal_risers.end() ? nullptr : &found->second;
}

/** @copydoc FindGeothermalRiserForPortal */
IndustryID FindGeothermalRiserForPortal(TileIndex portal)
{
	// Retain unfinished and abandoned attachments until actual demolition; a
	// second building must never multiply the sender's frozen200-kW capacity.
	for (const auto &[id, state] : _geothermal_risers) {
		if (state.site.portal == portal) return id;
	}
	return IndustryID::Invalid();
}

/** @copydoc PrepareGeothermalRiserConstruction */
GeothermalRiserConstructionAdmission PrepareGeothermalRiserConstruction(const GeothermalRiserRequest &request)
{
	GeothermalRiserConstructionAdmission result;
	result.foundation = PrepareGeothermalRiser(request);
	if (result.foundation.error != GeothermalError::None) return result;
	result.type = GetGeothermalRiserIndustryType();
	// Complete native identity/pool/binding capacity precedes the first cash or
	// tile mutation. A live attachment remains exclusive while awaiting supplies.
	if (result.type == IT_INVALID || !Industry::CanAllocateItem() || _geothermal_risers.size() >= MAX_GEOTHERMAL_SITE_CLAIMS ||
		FindGeothermalRiserForPortal(request.portal) != IndustryID::Invalid()) {
		result.foundation.error = GeothermalError::Limit;
		return result;
	}
	if (Money{result.foundation.construction->capital_cost} > GetAvailableMoneyForCommand()) result.foundation.error = GeothermalError::Refused;
	return result;
}

/** @copydoc BindGeothermalRiserIndustry */
bool BindGeothermalRiserIndustry(Industry &industry, const GeothermalRiserRequest &request)
{
	// The command has already acquired its native slot. Revalidation must not
	// demand a second free slot or permit rebinding a populated stock owner.
	if (Industry::GetIfValid(industry.index) != &industry || industry.location.w != 0 || _geothermal_risers.contains(industry.index) ||
		_geothermal_risers.size() >= MAX_GEOTHERMAL_SITE_CLAIMS || GetGeothermalRiserIndustryType() == IT_INVALID ||
		FindGeothermalRiserForPortal(request.portal) != IndustryID::Invalid()) return false;
	const auto admission = PrepareGeothermalRiser(request);
	if (admission.error != GeothermalError::None) return false;
	if (Money{admission.construction->capital_cost} > GetAvailableMoneyForCommand()) return false;
	return _geothermal_risers.emplace(industry.index, GeothermalRiserState{industry.index, request, admission.destination, admission.portal_revision}).second;
}

/** @param state Original paid construction/control association. @param industry Actual native machinery/stock. @return Whether the complete frozen layout, bill and construction flags agree, independently of temporary portal/company availability. */
static bool HasGeothermalRiserStructure(const GeothermalRiserState &state, const Industry &industry)
{
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser);
	// A native building cannot acquire a material receipt or move to another
	// floor merely by changing saved work flags or its translated definition.
	if (state.industry != industry.index || !IsGeothermalRiserDefinition(industry.type) || state.work_ticks > terms->work_ticks ||
		state.materials_committed != (state.work_ticks != 0) || industry.location.tile != state.site.tile ||
		industry.location.w != terms->width || industry.location.h != terms->height || industry.accepted.size() != terms->inputs.size() || !industry.produced.empty()) return false;
	// All four machinery cells share one paid completion boundary. Native tile
	// loops cannot freely finish a cell or hide an unexcavated/foreign floor.
	for (const auto &cell : GetIndustrySpec(industry.type)->layouts.front()) {
		const auto tile = TileAddWrap(state.site.tile, cell.ti.x, cell.ti.y);
		if (!IsValidTile(tile) || !industry.TileBelongsToIndustry(tile) || GetIndustryGfx(tile) != cell.gfx ||
			GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated || IsIndustryCompleted(tile) != (state.work_ticks == terms->work_ticks) ||
			GetIndustryConstructionStage(tile) != std::min<uint16_t>(3, state.work_ticks / 74)) return false;
	}
	// There is exactly one remaining native bill before consumption and zero
	// hidden stock afterward. Wrong or surplus meanings never become machinery.
	for (const auto &input : terms->inputs) {
		const auto slot = industry.GetCargoAccepted(GetCargoTypeByLabel(input.label));
		if (slot == industry.accepted.end() || slot->waiting > input.quantity || (state.materials_committed && slot->waiting != 0)) return false;
	}
	return true;
}

/** @param state Actual construction controller and original portal incarnation. @param industry Actual native floor/stock. @return Current calendar/company/two-endpoint readiness without rewriting the retained paid association. */
static bool IsGeothermalRiserAvailable(const GeothermalRiserState &state, const Industry &industry)
{
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	// Native industry ticks run for all companies. The current UI actor never
	// decides whether this paid machinery can earn synchronized labour.
	if (state.portal_retired || !Company::IsValidID(state.site.owner) || profile.id != MultiLayerGameplayProfileID::MultilayerIndustry || profile.legacy ||
		!CheckDeepLayerAvailability(profile, TimerGameCalendar::date) || !HasGeothermalRiserStructure(state, industry)) return false;
	const auto *portal = GetLayerPortalWorkRecord(state.site.portal);
	// Only explicit transitions of the same retained pair advance its stamp.
	// Loss is permanently tombstoned, so a fresh pair at the same coordinates
	// and even the same numeric revision cannot revive old paid machinery.
	if (portal == nullptr || portal->state != LayerPortalWorkState::Connected || portal->revision != state.portal_revision ||
		portal->company != state.site.owner || GetLayerPortalPairTile(state.site.portal) != state.destination ||
		GetTileOwner(state.site.portal) != state.site.owner || GetTileOwner(state.destination) != state.site.owner ||
		CheckLayerPortalAvailability(state.site.portal) != LayerPortalWorkError::None) return false;
	return true;
}

/** @copydoc GetGeothermalRiserInputRoom */
uint32_t GetGeothermalRiserInputRoom(const Industry &industry, CargoType cargo)
{
	const auto *state = GetGeothermalRiserState(industry.index);
	// Only an available unfinished native bill can accept material packets;
	// generic tile acceptance is never permission to discard overdelivery.
	if (state == nullptr || state->materials_committed || !IsValidCargoType(cargo)) return 0;
	if (!IsGeothermalRiserAvailable(*state, industry)) return 0;
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser);
	const auto input = std::ranges::find(terms->inputs, CargoSpec::Get(cargo)->label, &MultiLayerRecipeCargo::label);
	const auto slot = industry.GetCargoAccepted(cargo);
	// Cargo identities and native slots must agree before reading stock/history;
	// a forged acceptance cannot replace any part of the frozen four-input bill.
	if (input == terms->inputs.end() || slot == industry.accepted.end()) return 0;
	const auto accepted = slot->history == nullptr ? 0 : (*slot->history)[THIS_MONTH].accepted;
	return std::min<uint32_t>(input->quantity - slot->waiting, UINT16_MAX - accepted);
}

/** @copydoc ReceiveGeothermalRiserCargo */
uint32_t ReceiveGeothermalRiserCargo(Industry &industry, CargoType cargo, uint32_t offered)
{
	const auto amount = std::min(offered, GetGeothermalRiserInputRoom(industry, cargo));
	if (amount == 0) return 0;
	const auto slot = industry.GetCargoAccepted(cargo);
	auto &history = slot->GetOrCreateHistory(); // Allocate native history before publishing any accepted units.
	slot->waiting += static_cast<uint16_t>(amount);
	history[THIS_MONTH].accepted += static_cast<uint16_t>(amount);
	slot->last_accepted = TimerGameEconomy::date;
	industry.was_cargo_delivered = true;
	return amount;
}

/** @copydoc AdvanceGeothermalRiser */
void AdvanceGeothermalRiser(Industry &industry)
{
	const auto found = _geothermal_risers.find(industry.index);
	if (found == _geothermal_risers.end()) return;
	auto &state = found->second;
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser);
	// Prepaid construction earns one native tick only while the game, original
	// portal and full site are available. Pause/date loss never banks catch-up work.
	if (_game_mode != GameMode::Normal || _pause_mode.Any() || !IsGeothermalRiserAvailable(state, industry) || state.work_ticks == terms->work_ticks ||
		(state.has_work_tick && state.last_work_tick >= TimerGameTick::counter)) return;
	if (!state.materials_committed) {
		// Validate every arrived material before debiting any. Missing Batteries
		// retains Steel, Refined Metals and Electronics in the native stock owner.
		for (const auto &input : terms->inputs) {
			if (industry.GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting != input.quantity) return;
		}
		for (const auto &input : terms->inputs) industry.GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting -= static_cast<uint16_t>(input.quantity);
		state.materials_committed = true;
	}
	state.has_work_tick = true;
	state.last_work_tick = TimerGameTick::counter;
	++state.work_ticks;
	// The whole native2x2 building presents the same paid progress; completion
	// of its fourth296-tick stage creates no electricity or transfer identity.
	for (const auto &cell : GetIndustrySpec(industry.type)->layouts.front()) {
		const auto tile = TileAddWrap(state.site.tile, cell.ti.x, cell.ti.y);
		SetIndustryConstructionStage(tile, static_cast<uint8_t>(std::min<uint16_t>(3, state.work_ticks / 74)));
		if (state.work_ticks == terms->work_ticks) SetIndustryCompleted(tile);
		MarkTileDirtyByTile(tile);
	}
}

/** @copydoc IsGeothermalRiserOperational */
bool IsGeothermalRiserOperational(IndustryID industry, bool require_normal_game)
{
	const auto *state = GetGeothermalRiserState(industry);
	const auto *native = Industry::GetIfValid(industry);
	// Reused/deleted native IDs and unfinished receipts cannot stand in for the
	// complete paid original machinery when the later exporter asks for capacity.
	if (state == nullptr || native == nullptr) return false;
	return (!require_normal_game || _game_mode == GameMode::Normal) && state->materials_committed &&
		state->work_ticks == GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->work_ticks && IsGeothermalRiserAvailable(*state, *native);
}

/** @copydoc GeothermalRiserHasStoredCargo */
bool GeothermalRiserHasStoredCargo(const Industry &industry)
{
	// Native saved stock is protected even before its machinery association is
	// accepted on reload; malformed output cannot disappear through demolition.
	if (GetGeothermalRiserState(industry.index) == nullptr && !IsGeothermalRiserDefinition(industry.type)) return false;
	return std::ranges::any_of(industry.accepted, [](const auto &slot) { return slot.waiting != 0; }) ||
		std::ranges::any_of(industry.produced, [](const auto &slot) { return slot.waiting != 0; });
}

/** @copydoc ReleaseGeothermalRiserIndustry */
void ReleaseGeothermalRiserIndustry(IndustryID industry)
{
	InvalidateUtilityPowerLinksForRiser(industry); // Physical machinery loss retires its paid electrical identity before a native IndustryID can be reused.
	_geothermal_risers.erase(industry);
}

/** @copydoc RevalidateGeothermalRiserPortal */
void RevalidateGeothermalRiserPortal(TileIndex portal, CompanyID previous_owner, uint64_t previous_revision)
{
	const auto key = GetLayerPortalPairKey(portal);
	if (!key) return;
	const auto pair = GetLayerPortalPairTile(portal);
	const auto *record = GetLayerPortalWorkRecord(portal);
	// This callback belongs to the native committed transition, while both map
	// ends still have their preceding owner. Queries or a later coincident pair
	// cannot adopt machinery by guessing a company or a newer revision.
	if (record == nullptr || previous_revision >= UINT64_MAX - 1 || record->revision != previous_revision + 1 ||
		GetTileOwner(portal) != previous_owner || GetTileOwner(pair) != previous_owner) return;
	// Iterate the bounded attachments only. Both directions may own separate
	// paid machinery, but a retired or already stale attachment never rejoins.
	for (auto &[id, state] : _geothermal_risers) {
		if (state.portal_retired || state.portal_revision != previous_revision ||
			(state.site.portal != portal && state.site.portal != pair) ||
			(state.site.owner != previous_owner && state.site.owner != record->company)) continue;
		if (GetLayerPortalPairTile(state.site.portal) != state.destination) continue;
		state.portal_revision = record->revision;
		state.site.owner = record->company;
		if (!Company::IsValidID(record->company)) state.portal_retired = true;
	}
}

/** @copydoc RetireGeothermalRiserPortal */
void RetireGeothermalRiserPortal(TileIndex portal)
{
	// Physical demolition permanently revokes both original-direction rights.
	// Keep the stamp and native receipt/work/stock for recovery/inspection; a
	// new housing may reuse coordinates and start its revision sequence again.
	for (auto &[id, state] : _geothermal_risers) {
		if (state.site.portal == portal || state.destination == portal) state.portal_retired = true;
	}
}

/** @copydoc ChangeGeothermalRiserOwnership */
void ChangeGeothermalRiserOwnership(Owner old_owner, Owner new_owner)
{
	// Actual takeover/abandonment changes only the controller. Paid work,
	// delivered stock and original portal incarnation must survive unchanged.
	for (auto &[id, state] : _geothermal_risers) {
		if (state.site.owner != old_owner) continue;
		state.site.owner = Company::IsValidID(new_owner) ? CompanyID{new_owner} : CompanyID::Invalid();
		if (!Company::IsValidID(new_owner)) state.portal_retired = true; // Abandonment cannot later adopt a reused company ID or ownerless housing.
	}
}

/** @copydoc ResetGeothermalRisers */
void ResetGeothermalRisers()
{
	_geothermal_risers.clear();
	_pending_geothermal_risers.reset();
	ResetGeothermalRiserLoadStaging();
}

/** @copydoc CaptureGeothermalRisers */
std::vector<GeothermalRiserState> CaptureGeothermalRisers()
{
	std::vector<GeothermalRiserState> rows; rows.reserve(_geothermal_risers.size());
	// Stable native ID order transports only associations/work; no private
	// material balance or electrical allocation is introduced by this snapshot.
	for (const auto &[id, state] : _geothermal_risers) rows.push_back(state);
	return rows;
}

/** @copydoc ValidateGeothermalRisers */
bool ValidateGeothermalRisers(std::span<const GeothermalRiserState> rows, bool world_refs)
{
	if (rows.size() > MAX_GEOTHERMAL_SITE_CLAIMS) return false;
	std::set<IndustryID> identities; std::set<TileIndex> senders;
	// Validate the whole bounded family before publishing any row. Tombstones
	// preserve original references but never waive native machinery/stock checks.
	for (const auto &state : rows) {
		if (!IsValidTile(state.site.tile) || !IsValidTile(state.site.portal) || !IsValidTile(state.site.port) || !IsValidTile(state.destination) ||
			!identities.insert(state.industry).second || !senders.insert(state.site.portal).second) return false;
		if (GetTileLayer(state.site.tile) != GetTileLayer(state.site.portal) || GetTileLayer(state.site.port) != GetTileLayer(state.site.tile)) return false; // All construction cells retain one real rock plane; no coincident cross-layer floor is admitted.
		if (!ValidateGeothermalRiserWire(EncodeGeothermalRiser(state), Map::SizeX(), Map::SizeY(), Map::LayerCount())) return false;
		if (!world_refs) continue;
		const auto *industry = Industry::GetIfValid(state.industry);
		// Native floor/stock/stage and the saved synchronized clock must agree;
		// a material receipt cannot authorize freely completed or missing cells.
		if (industry == nullptr || !HasGeothermalRiserStructure(state, *industry) ||
			(state.site.owner != CompanyID::Invalid() && !Company::IsValidID(state.site.owner)) ||
			(state.has_work_tick && state.last_work_tick > TimerGameTick::counter)) return false;
		if (state.portal_retired) continue;
		const auto *portal = GetLayerPortalWorkRecord(state.site.portal);
		// Live or temporarily stale attachments keep their original exact stamp.
		// A later revision may suspend them, but load never restamps or repairs it.
		if (portal == nullptr || portal->company != state.site.owner || portal->revision < state.portal_revision ||
			GetLayerPortalPairTile(state.site.portal) != state.destination || GetTileOwner(state.site.portal) != state.site.owner || GetTileOwner(state.destination) != state.site.owner) return false;
	}
	if (!world_refs) return true;
	// Exact native-family coverage catches a malicious empty/omitted table
	// even if its metadata feature bit was cleared at the same time.
	for (const auto *industry : Industry::Iterate()) {
		if (IsGeothermalRiserDefinition(industry->type) && !identities.contains(industry->index)) return false;
	}
	return true;
}

/** @copydoc StageLoadedGeothermalRisers */
void StageLoadedGeothermalRisers(std::vector<GeothermalRiserState> rows) { _pending_geothermal_risers = std::move(rows); }

/** @copydoc AcceptLoadedGeothermalRisers */
bool AcceptLoadedGeothermalRisers()
{
	const auto rows = _pending_geothermal_risers ? std::span<const GeothermalRiserState>{*_pending_geothermal_risers} : std::span<const GeothermalRiserState>{};
	if (!ValidateGeothermalRisers(rows)) return false;
	std::map<IndustryID, GeothermalRiserState> accepted;
	// Construct the complete validated family before replacing any live binding.
	for (const auto &state : rows) accepted.emplace(state.industry, state);
	_geothermal_risers = std::move(accepted); _pending_geothermal_risers.reset();
	return true;
}

/** @copydoc ComputeGeothermalRiserDigest */
uint64_t ComputeGeothermalRiserDigest()
{
	uint64_t hash = 14695981039346656037ULL;
	/** @param value Explicit authoritative scalar, encoded without host padding or pointers. */
	const auto field = [&hash](uint64_t value) { for (uint byte = 0; byte < 8; ++byte) { hash ^= value & 0xFF; hash *= 1099511628211ULL; value >>= 8; } };
	field(_geothermal_risers.size());
	// Read each actual participant and its native stock/history in canonical
	// order. Diagnostics do not create a second inventory or energy authority.
	for (const auto &[id, state] : _geothermal_risers) {
		field(id.base()); field(state.site.owner.base()); field(state.site.tile.base()); field(state.site.portal.base()); field(state.site.port.base()); field(to_underlying(state.site.access));
		field(state.destination.base()); field(state.portal_revision); field(state.portal_retired); field(state.work_ticks);
		field(state.materials_committed); field(state.has_work_tick); field(state.last_work_tick);
		const auto *industry = Industry::GetIfValid(id);
		if (industry == nullptr) { field(UINT64_MAX); continue; }
		for (const auto &input : industry->accepted) {
			field(input.cargo); field(input.waiting); field(input.last_accepted.base());
			if (input.history == nullptr) { field(UINT64_MAX); continue; }
			for (const auto &month : *input.history) field(month.accepted); // Retained native receipt history remains observable after whole-bill consumption.
		}
	}
	return hash;
}
