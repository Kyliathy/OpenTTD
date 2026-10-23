/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_industry_mine.cpp Stable mine bindings coordinated with the existing geological owner. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Groups 4-8/15/16 */

#include "stdafx.h"
#include "multilayer_industry.h"
#include "industry.h"
#include "company_base.h"
#include "openttd.h"
#include "economy_func.h"
#include "layer_industry_processor.h"

#include "safeguards.h"

static std::map<IndustryID, LayerMineState> _layer_mines; ///< One binding per real IndustryID; no reserve or physical cargo is duplicated.
static std::optional<std::vector<LayerMineState>> _staged_layer_mines; ///< Unpublished loaded bindings; discarded on world reset or rejected acceptance.

/** @copydoc GetLayerMineState */
const LayerMineState *GetLayerMineState(IndustryID industry)
{
	const auto found = _layer_mines.find(industry);
	return found == _layer_mines.end() ? nullptr : &found->second;
}

/** @copydoc FindLayerMineIndustry */
IndustryID FindLayerMineIndustry(GeologyDepositID deposit)
{
	// A physical binding remains exclusive after abandonment; an invalid company claim cannot authorize a second mine.
	for (const auto &[industry, mine] : _layer_mines) {
		if (mine.deposit == deposit) return industry;
	}
	return IndustryID::Invalid();
}

/** @copydoc ChangeLayerIndustryOwnership */
void ChangeLayerIndustryOwnership(CompanyID old_company, CompanyID new_company)
{
	// Change only bindings actually owned by the removed company; physical output remains in the same Industry cargo slots.
	for (auto &[industry, mine] : _layer_mines) {
		if (mine.company != old_company) continue;
		const auto transferred = TransferLayerMineDeposit(mine.deposit, old_company, new_company);
		assert(transferred == GeologyError::None);
		if (transferred != GeologyError::None) continue;
		mine.company = new_company;
		if (new_company == CompanyID::Invalid()) mine.phase = LayerMinePhase::Abandoned;
		if (mine.revision != UINT64_MAX) ++mine.revision; // Exhausted quotes stay permanently inert across takeover/removal instead of wrapping.
	}
	ChangeLayerProcessorControl(old_company, new_company);
}

/** @copydoc BindLayerMineIndustry */
LayerIndustryError BindLayerMineIndustry(Industry &industry, const LayerMineRequest &request, const LayerMineAdmission &admission)
{
	// Only the complete zero-footprint construction transaction may reserve a binding; estimates cannot enter here.
	if (admission.error != LayerIndustryError::None || industry.location.w != 0 ||
			_layer_mines.contains(industry.index) || _layer_mines.size() >= MAX_GEOLOGY_DEPOSITS ||
			!IsLayerMineDefinition(request.type)) return LayerIndustryError::Capacity;
	const auto [position, inserted] = _layer_mines.emplace(industry.index,
		LayerMineState{industry.index, request.deposit, request.company});
	if (!inserted) return LayerIndustryError::Capacity;
	const auto claimed = ClaimLayerMineDeposit(request.deposit, request.company, admission.expected_reserve);
	if (claimed != GeologyError::None) {
		_layer_mines.erase(position);
		return claimed == GeologyError::Depleted ? LayerIndustryError::Depleted : LayerIndustryError::Claimed;
	}
	return LayerIndustryError::None;
}

/** @copydoc ReleaseLayerMineIndustry */
void ReleaseLayerMineIndustry(IndustryID industry)
{
	const auto found = _layer_mines.find(industry);
	if (found == _layer_mines.end()) return;
	ReleaseLayerMineDeposit(found->second.deposit, found->second.company);
	_layer_mines.erase(found);
}

/** @copydoc LayerMineHasStoredOutput */
bool LayerMineHasStoredOutput(const Industry &industry)
{
	// A restoring native mine must retain finite output even before its validated binding rows are accepted.
	if (GetLayerMineState(industry.index) == nullptr && !IsLayerMineDefinition(industry.type)) return false;
	return std::ranges::any_of(industry.produced, [](const auto &cargo) { return cargo.waiting != 0; });
}

/** @copydoc ResetLayerIndustryState */
void ResetLayerIndustryState()
{
	_layer_mines.clear();
	_staged_layer_mines.reset();
	ResetLayerMineLoadStaging();
	ResetLayerProcessorState();
}

/** @copydoc CaptureLayerMines */
std::vector<LayerMineState> CaptureLayerMines()
{
	std::vector<LayerMineState> result;
	result.reserve(_layer_mines.size());
	// Stable industry order is save transport, never a replacement for geology's stable deposit identity.
	for (const auto &[id, mine] : _layer_mines) result.push_back(mine);
	return result;
}

/** @copydoc StageLoadedLayerMines */
void StageLoadedLayerMines(std::vector<LayerMineState> rows)
{
	_staged_layer_mines = std::move(rows);
}

/**
 * Validate a restored binding without reclaiming or recreating geological material.
 * @param mine Saved binding and historical counters.
 * @param industry Rebuilt physical stock owner.
 * @return Whether immutable recipe, geometry, claim, lifecycle, work and finite material agree.
 */
static bool IsLoadedLayerMineValid(const LayerMineState &mine, const Industry &industry)
{
	const auto *capability = GetLayerIndustryCapability(industry.type);
	const auto *deposit = GetGeologyWorldState().deposits.Get(mine.deposit);
	// Only an exact native mineral, on its actual deposit's allowed layer, can own this saved binding.
	if (capability == nullptr || capability->resource == CT_INVALID || deposit == nullptr ||
			!IsValidTile(industry.location.tile) || GetTileLayer(industry.location.tile) != deposit->layer ||
			!capability->layers.Test(GetTileLayer(industry.location.tile)) || deposit->resource != capability->resource ||
			industry.produced.size() != 1 || !industry.accepted.empty() || industry.location.w == 0 || industry.location.h == 0) return false;
	const auto &output = industry.produced.front();
	const auto recipes = GetMultiLayerMineRecipes();
	const auto recipe = std::ranges::find(recipes, capability->recipe, [](const auto &item) { return item.recipe.id; });
	// Runtime cargo slots must still mean this mineral; stale labels or oversized stock cannot be silently translated.
	if (!IsValidCargoType(output.cargo) || CargoSpec::Get(output.cargo)->label != capability->resource ||
			output.waiting > capability->output_capacity || recipe == recipes.end() || mine.revision == 0 ||
			mine.phase > LayerMinePhase::Abandoned || mine.scale_fraction >= 200 || mine.dispatched > mine.extracted ||
			mine.extracted - mine.dispatched != output.waiting || deposit->remaining_reserve > deposit->initial_reserve ||
			mine.extracted > deposit->initial_reserve - deposit->remaining_reserve) return false;
	const auto &batch = recipe->recipe;
	const TileIndex last = TileAddWrap(industry.location.tile, industry.location.w - 1, industry.location.h - 1);
	// Bound restored rectangles to the admitted layout size and one plane before walking their actual machinery.
	if (static_cast<uint32_t>(industry.location.w) * industry.location.h > 64 || !IsValidTile(last) || GetTileLayer(last) != deposit->layer) return false;
	bool contact = false;
	// A saved claim still requires real excavated machinery touching its recorded seam; IDs alone cannot relocate a mine.
	for (TileIndex tile : industry.location) {
		if (!industry.TileBelongsToIndustry(tile)) continue;
		if (GetGeologyWorldState().terrain.Get(tile) != ExcavationState::Excavated) return false;
		contact |= GetGeologyWorldState().deposits.Find(tile) == mine.deposit;
	}
	if (!contact) return false;
	// Banked work is at most one frozen interval, including its rational remainder, without new production on load.
	if (mine.work_ticks > batch.work_economy_ticks || mine.work_fraction >= batch.outputs.front().quantity ||
			(mine.work_ticks == batch.work_economy_ticks && mine.work_fraction != 0)) return false;
	if (mine.phase == LayerMinePhase::Abandoned) return mine.company == CompanyID::Invalid() && deposit->claimant == CompanyID::Invalid();
	// Active/exhausted machinery retains its original claimant, even when the date now forbids additional extraction.
	if (!Company::IsValidID(mine.company) || deposit->claimant != mine.company) return false;
	return (mine.phase == LayerMinePhase::Depleted) == (deposit->remaining_reserve == 0);
}

/** @copydoc AcceptLoadedLayerMines */
bool AcceptLoadedLayerMines()
{
	const auto rows = _staged_layer_mines ? std::move(*_staged_layer_mines) : CaptureLayerMines();
	_staged_layer_mines.reset();
	if (rows.size() > MAX_GEOLOGY_DEPOSITS) return false;
	std::map<IndustryID, LayerMineState> accepted;
	std::set<GeologyDepositID> deposits;
	// Validate all references and exclusion first; one invalid record cannot publish part of a new family.
	for (const auto &mine : rows) {
		const Industry *industry = Industry::GetIfValid(mine.industry);
		if (industry == nullptr) return false;
		if (!IsLoadedLayerMineValid(mine, *industry)) return false;
		if (!accepted.emplace(mine.industry, mine).second || !deposits.insert(mine.deposit).second) return false; // A physical mine and seam each have one binding.
	}
	// A native mine without a saved row cannot run on fabricated defaults or a regenerated claim.
	for (const Industry *industry : Industry::Iterate()) {
		if (IsLayerMineDefinition(industry->type) && !accepted.contains(industry->index)) return false;
	}
	_layer_mines = std::move(accepted);
	return true;
}

/** @copydoc ComputeLayerMineDigest */
uint64_t ComputeLayerMineDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	// Explicit ordered integers exclude pointer addresses, padding and transient station quotes from diagnostics.
	for (const auto &[id, mine] : _layer_mines) {
		for (uint64_t value : {uint64_t{id.base()}, uint64_t{mine.deposit.base()}, uint64_t{mine.company.base()},
				uint64_t{to_underlying(mine.phase)}, mine.work_ticks, mine.extracted, mine.dispatched, mine.revision,
				uint64_t{mine.work_fraction}, uint64_t{mine.scale_fraction}}) {
			for (int shift = 56; shift >= 0; shift -= 8) {
				digest ^= static_cast<uint8_t>(value >> shift);
				digest *= 1099511628211ULL;
			}
		}
	}
	return digest;
}

/**
 * Verify that all physical machinery of a bound mine has finished construction.
 * @param industry Actual placed industry, whose rectangle stays in its own world layer.
 * @return Whether at least one machinery tile exists and every such tile is complete.
 */
bool IsLayerIndustryComplete(const Industry &industry)
{
	bool found = false;
	// Layout rectangles can contain unused cells; only machinery owned by this industry's identity blocks production.
	for (TileIndex tile : industry.location) {
		if (!industry.TileBelongsToIndustry(tile)) continue;
		found = true;
		if (!IsIndustryCompleted(tile)) return false;
	}
	return found;
}

/** @copydoc GetLayerMineOperationalStatus */
LayerMineOperationalStatus GetLayerMineOperationalStatus(const Industry &industry)
{
	const auto *mine = GetLayerMineState(industry.index);
	const auto *capability = GetLayerIndustryCapability(industry.type);
	// An unknown/restoring binding or non-mine capability cannot present another inventory's reserve or permission as mine operation.
	if (mine == nullptr || capability == nullptr || capability->resource == CT_INVALID) return LayerMineOperationalStatus::MissingState;
	const auto *deposit = GetGeologyWorldState().deposits.Get(mine->deposit);
	if (deposit == nullptr) return LayerMineOperationalStatus::MissingState;
	if (!IsLayerIndustryComplete(industry)) return LayerMineOperationalStatus::Construction;
	// Abandoned machinery retains its geological/output accounting while extraction waits for a valid mining company.
	if (mine->phase == LayerMinePhase::Abandoned || !Company::IsValidID(mine->company)) return LayerMineOperationalStatus::Abandoned;
	if (!IsLayerIndustryTechnologyAvailable(*capability, GetTileLayer(industry.location.tile), GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return LayerMineOperationalStatus::Unavailable;
	// Depletion is a permanent reserve state even while the last stored output awaits ordinary station pickup.
	if (mine->phase == LayerMinePhase::Depleted || deposit->remaining_reserve == 0) return LayerMineOperationalStatus::Depleted;
	if (industry.prod_level == PRODLEVEL_CLOSURE) return LayerMineOperationalStatus::Stopped;
	if (_pause_mode.Any() || _game_mode != GameMode::Normal) return LayerMineOperationalStatus::Paused; // Frozen/editor worlds retain material and work.
	if (industry.produced.size() != 1) return LayerMineOperationalStatus::MissingState;
	const auto &output = industry.produced.front();
	if (output.waiting >= capability->output_capacity) return LayerMineOperationalStatus::OutputFull;
	// A pure station quote distinguishes real service/capacity refusal from reserve extraction; it allocates no packets or station fractions.
	if (output.waiting != 0 && PrepareLayerMineDispatch(industry).station.amount == 0) return LayerMineOperationalStatus::AwaitPickup;
	return LayerMineOperationalStatus::Operating;
}

/** @copydoc ProduceLayerMineCargo */
LayerMineProduction ProduceLayerMineCargo(Industry &industry, uint32_t economy_ticks, bool execute)
{
	const auto position = _layer_mines.find(industry.index);
	if (position == _layer_mines.end()) return {LayerIndustryError::InvalidDefinition};
	LayerMineState &mine = position->second;
	const auto *capability = GetLayerIndustryCapability(industry.type);
	// A bound geological industry must still have its exact native definition and physical output slot.
	if (capability == nullptr || capability->resource == CT_INVALID || industry.produced.size() != 1) return {LayerIndustryError::InvalidDefinition};
	if (!Company::IsValidID(mine.company)) return {LayerIndustryError::InvalidCompany};
	if (mine.phase == LayerMinePhase::Abandoned) return {LayerIndustryError::InvalidCompany};
	if (mine.phase == LayerMinePhase::Depleted) return {LayerIndustryError::Depleted};
	const MapLayer layer = GetTileLayer(industry.location.tile);
	if (!IsLayerIndustryTechnologyAvailable(*capability, layer, GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return {LayerIndustryError::Unavailable};
	// Pausing, scenario editing, unfinished machinery and a script's stopped production preserve stock and work.
	if (_pause_mode.Any() || _game_mode != GameMode::Normal || industry.prod_level == PRODLEVEL_CLOSURE || !IsLayerIndustryComplete(industry)) return {};
	const auto recipes = GetMultiLayerMineRecipes();
	const auto record = std::ranges::find(recipes, capability->recipe, [](const auto &item) { return item.recipe.id; });
	if (record == recipes.end()) return {LayerIndustryError::InvalidDefinition};
	const MultiLayerRecipe &recipe = record->recipe;
	const GeologyDeposit *deposit = GetGeologyWorldState().deposits.Get(mine.deposit);
	// The sole reserve owner's identity, meaning, layer and claimant must still agree with this physical binding.
	if (deposit == nullptr || deposit->resource != capability->resource || deposit->layer != layer || deposit->claimant != mine.company) return {LayerIndustryError::InvalidDeposit};
	if (deposit->remaining_reserve == 0) return {LayerIndustryError::Depleted};
	Industry::ProducedCargo &output = industry.produced.front();
	// Resolving the content label here prevents a changed runtime cargo slot from receiving another mineral's debit.
	if (!IsValidCargoType(output.cargo) || CargoSpec::Get(output.cargo)->label != capability->resource || ValidateMultiLayerRecipe(recipe) != MultiLayerRecipeLimit::None) return {LayerIndustryError::InvalidDefinition};
	const uint64_t quantity = recipe.outputs.front().quantity;
	const uint64_t work_cap = recipe.work_economy_ticks;
	// Saved work is at most one recipe interval; invalid carries or counters cannot wrap into renewed production.
	if (mine.work_ticks > work_cap || mine.work_fraction >= quantity || output.waiting > capability->output_capacity ||
			mine.revision == 0 || mine.revision == UINT64_MAX || mine.extracted == UINT64_MAX || mine.scale_fraction >= 200) return {LayerIndustryError::Capacity};
	if (output.waiting == capability->output_capacity) return {};
	const uint64_t credit_cap = work_cap * quantity;
	const uint64_t old_credit = mine.work_ticks * quantity + mine.work_fraction;
	if (old_credit > credit_cap) return {LayerIndustryError::Capacity};
	// Apply the declared rate and recession to earned work, before any finite material exists; retain deterministic rounding.
	const uint64_t scaled_work = static_cast<uint64_t>(economy_ticks) * _settings_game.economy.industry_cargo_scale * (EconomyIsInRecession() ? 1 : 2) + mine.scale_fraction;
	const uint16_t scale_fraction = static_cast<uint16_t>(scaled_work % 200);
	const uint64_t credit = std::min(credit_cap, old_credit + (scaled_work / 200) * quantity);
	const uint64_t amount = std::min({credit / work_cap, quantity, deposit->remaining_reserve,
		static_cast<uint64_t>(capability->output_capacity - output.waiting), UINT64_MAX - mine.extracted,
		static_cast<uint64_t>(UINT16_MAX - output.history[THIS_MONTH].production)});
	const uint64_t charged_credit = amount * work_cap;
	const uint64_t remaining_credit = credit - charged_credit;
	LayerMineProduction result{LayerIndustryError::None, static_cast<uint32_t>(amount), charged_credit / quantity,
		static_cast<uint32_t>(charged_credit % quantity)};
	if (!execute) return result;
	if (amount != 0) {
		const auto error = ConsumeLayerMineReserve(mine.deposit, mine.company, amount, deposit->remaining_reserve);
		if (error != GeologyError::None) return {LayerIndustryError::InvalidDeposit};
		// All fallible participants finished before these non-allocating writes: every debited unit reaches actual output.
		output.waiting += static_cast<uint16_t>(amount);
		output.history[THIS_MONTH].production += static_cast<uint16_t>(amount);
		mine.extracted += amount;
		industry.last_prod_year = TimerGameEconomy::year;
		if (deposit->remaining_reserve == 0) mine.phase = LayerMinePhase::Depleted;
	}
	// Rational work preserves the frozen rate when buffer/reserve bounds admit less than a complete output batch.
	if (old_credit != remaining_credit || mine.scale_fraction != scale_fraction || amount != 0) {
		mine.work_ticks = remaining_credit / quantity;
		mine.work_fraction = static_cast<uint32_t>(remaining_credit % quantity);
		mine.scale_fraction = scale_fraction;
		++mine.revision;
	}
	return result;
}

/** @copydoc PrepareLayerMineDispatch */
LayerMineDispatchTicket PrepareLayerMineDispatch(const Industry &industry)
{
	LayerMineDispatchTicket ticket;
	const LayerMineState *mine = GetLayerMineState(industry.index);
	if (mine == nullptr) return ticket;
	// Abandonment stops extraction but permits recovery of already mined cargo through any real same-layer station.
	// Once empty, ordinary demolition/rebuild can claim only the original remaining reserve.
	if (!Company::IsValidID(mine->company) && mine->phase != LayerMinePhase::Abandoned) return ticket;
	const auto *capability = GetLayerIndustryCapability(industry.type);
	// Only a valid bound mineral output can issue an integer station ticket; a full historical counter cannot wrap.
	if (capability == nullptr || capability->resource == CT_INVALID || industry.produced.size() != 1 || mine->revision == UINT64_MAX) return ticket;
	const MapLayer layer = GetTileLayer(industry.location.tile);
	if (!IsLayerIndustryTechnologyAvailable(*capability, layer, GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return ticket;
	// Frozen/unfinished worlds retain output; depletion alone does not prevent the last mined material from leaving.
	if (_pause_mode.Any() || _game_mode != GameMode::Normal || !IsLayerIndustryComplete(industry)) return ticket;
	const auto &output = industry.produced.front();
	if (!IsValidCargoType(output.cargo)) return ticket;
	if (output.waiting > capability->output_capacity) return ticket;
	if (CargoSpec::Get(output.cargo)->label != capability->resource) return ticket;
	const uint32_t offered = static_cast<uint32_t>(std::min({static_cast<uint64_t>(output.waiting), uint64_t{255},
		UINT64_MAX - mine->dispatched, static_cast<uint64_t>(UINT16_MAX - output.history[THIS_MONTH].transported)}));
	if (offered == 0) return ticket;
	ticket.industry = industry.index;
	ticket.revision = mine->revision;
	ticket.waiting = output.waiting;
	ticket.transported = output.history[THIS_MONTH].transported;
	// An abandoned mining claim no longer owns pickup, but any surviving script-selected transport exclusivity still applies.
	const Owner pickup_owner = mine->phase == LayerMinePhase::Abandoned ? industry.exclusive_consumer : mine->company;
	// Select real service by rating, with stable station identity breaking ties; no vertical catchment or foreign company is offered stock.
	for (const Station *station : industry.stations_near) {
		auto candidate = PrepareExactStationSupply(station->index, output.cargo, offered, {industry.index, SourceType::Industry}, layer, pickup_owner);
		if (candidate.amount == 0) continue;
		// Rating limits throughput, never destroys finite stock; round the last positive unit upward to avoid stranding it forever.
		candidate.amount = static_cast<uint16_t>(std::min<uint32_t>(candidate.amount, (offered * (candidate.rating + 1) + 255) / 256));
		if (ticket.station.amount == 0 || candidate.rating > ticket.station.rating ||
				(candidate.rating == ticket.station.rating && candidate.station < ticket.station.station)) ticket.station = candidate;
	}
	return ticket;
}

/** @copydoc CommitLayerMineDispatch */
uint32_t CommitLayerMineDispatch(Industry &industry, const LayerMineDispatchTicket &ticket)
{
	if (ticket.station.amount == 0) return 0;
	const auto expected = PrepareLayerMineDispatch(industry);
	// Revalidate both actual owners and the chosen service before any packet write or mine-output debit.
	if (ticket.industry != expected.industry || ticket.revision != expected.revision || ticket.waiting != expected.waiting ||
			ticket.transported != expected.transported || ticket.station.station != expected.station.station ||
			ticket.station.cargo != expected.station.cargo || ticket.station.source != expected.station.source ||
			ticket.station.layer != expected.station.layer || ticket.station.exclusivity != expected.station.exclusivity ||
			ticket.station.waiting != expected.station.waiting || ticket.station.rating != expected.station.rating ||
			ticket.station.amount != expected.station.amount) return 0;
	const uint32_t supplied = CommitExactStationSupply(ticket.station);
	if (supplied == 0) return 0;
	LayerMineState &mine = _layer_mines.at(industry.index);
	Industry::ProducedCargo &output = industry.produced.front();
	// The station owns exactly this many complete packet units; these already-admitted non-failing source writes match its receipt.
	output.waiting -= static_cast<uint16_t>(supplied);
	output.history[THIS_MONTH].transported += static_cast<uint16_t>(supplied);
	mine.dispatched += supplied;
	++mine.revision;
	return supplied;
}

/** @copydoc DispatchLayerMineCargo */
uint32_t DispatchLayerMineCargo(Industry &industry)
{
	return CommitLayerMineDispatch(industry, PrepareLayerMineDispatch(industry));
}
