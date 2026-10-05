/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file spoil_receiver.cpp The Surface Earth reclamation site and Rock crusher (Plan 5 Group 8). Their industry and
 * tile definitions are installed natively under the first-party package's GRF ID, as the TBM engines are; their
 * bounded intake, fees, interval processing and saved side state live here, beside the industry's own stock.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 8
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-i-receiver-design.md
 */

#include "stdafx.h"
#include "spoil_receiver.h"
#include "industry.h"
#include "industrytype.h"
#include "newgrf_commons.h"
#include "station_base.h"
#include "cargotype.h"
#include "company_base.h"
#include "company_func.h"
#include "command_func.h"
#include "multilayer_content.h"
#include "multilayer_content_identity.h"
#include "window_func.h"
#include "core/math_func.hpp"
#include "table/strings.h"

#include "safeguards.h"

/** Extern declaration for _origin_industry_specs in table/build_industry.h, the untouched original definitions. */
extern const IndustrySpec _origin_industry_specs[NEW_INDUSTRYOFFSET];

/* Original industry types used as templates (IndustryTypes in table/build_industry.h). */
static constexpr IndustryType ORIGINAL_COAL_MINE = 0; ///< Art template of the Earth site.
static constexpr IndustryType ORIGINAL_POWER_STATION = 1; ///< Behaviour template: an accept-only "black hole".
static constexpr IndustryType ORIGINAL_IRON_MINE = 18; ///< Art template of the Rock crusher.

static constexpr uint16_t SPOIL_RECEIVER_INDUSTRY_LOCAL_BASE = 0x8000; ///< Reserved native industry range in the package's GRF scope.
static constexpr uint16_t SPOIL_RECEIVER_TILE_LOCAL_BASE = 0x8000; ///< Reserved native industry tile range in the package's GRF scope.
static constexpr uint16_t SPOIL_RECEIVER_TILES_PER_KIND = 16; ///< Tile IDs reserved per kind.

/** The frozen recipes of r2mlvs1-d Group 7, indexed by kind. */
static const SpoilReceiverRecipe _spoil_receiver_recipes[] = {
	{CargoLabel{"ERTH"}, 8, 1024, 32, 2, 0, 0, 0, 0},
	{CargoLabel{"ROCK"}, 8, 1024, 64, 3, 4, 6, 2, 1024},
};
static_assert(std::size(_spoil_receiver_recipes) == to_underlying(SpoilReceiverKind::End));

static std::map<IndustryID, SpoilReceiver> _spoil_receivers; ///< Live receiver state by industry.
static std::optional<std::vector<SpoilReceiver>> _staged_spoil_receivers; ///< Loaded rows awaiting AcceptLoadedSpoilReceivers.

const SpoilReceiverRecipe &GetSpoilReceiverRecipe(SpoilReceiverKind kind)
{
	assert(kind < SpoilReceiverKind::End);
	return _spoil_receiver_recipes[to_underlying(kind)];
}

/** @return The first-party package's GRF ID, the scope of both native definitions. */
static GrfID GetSpoilReceiverGrfID()
{
	return GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package.grfid;
}

/**
 * Build the native tile for one original art tile: its look comes from the substitute, and it accepts the spoil fully.
 * @param original Original tile graphics to draw.
 * @param local Reserved local tile ID.
 * @param cargo Accepted spoil.
 * @param label Its label.
 * @return The tile definition.
 */
static IndustryTileSpec MakeSpoilReceiverTile(IndustryGfx original, uint16_t local, CargoType cargo, CargoLabel label)
{
	IndustryTileSpec tile = *GetIndustryTileSpec(original);
	tile.accepts_cargo.fill(INVALID_CARGO);
	tile.acceptance.fill(0);
	tile.accepts_cargo[0] = cargo;
	tile.acceptance[0] = 8; // A whole 8/8 per tile: any catchment touching the site accepts it.
	tile.accepts_cargo_label.fill(CT_INVALID);
	tile.accepts_cargo_label[0] = label;
	tile.anim_production = INDUSTRYTILE_NOANIM;
	tile.anim_next = INDUSTRYTILE_NOANIM;
	tile.anim_state = false;
	tile.callback_mask = {};
	tile.animation = {};
	tile.special_flags = {};
	tile.enabled = true;
	tile.grf_prop = SubstituteGRFFileProps(original);
	tile.grf_prop.override_id = INVALID_INDUSTRYTILE;
	tile.grf_prop.local_id = local;
	tile.grf_prop.grfid = GetSpoilReceiverGrfID();
	return tile;
}

/**
 * Install one receiver's tiles and industry definition.
 * @param kind Receiver kind.
 * @return Whether both are installed in their saved or newly reserved slots.
 */
static bool InstallSpoilReceiverDefinition(SpoilReceiverKind kind)
{
	const auto &recipe = GetSpoilReceiverRecipe(kind);
	const CargoType cargo = GetCargoTypeByLabel(recipe.cargo);
	if (!IsValidCargoType(cargo)) return false;
	const GrfID grfid = GetSpoilReceiverGrfID();
	const bool earth = kind == SpoilReceiverKind::Earth;
	const IndustryTileLayout &art = earth ? _origin_industry_specs[ORIGINAL_COAL_MINE].layouts[2] : _origin_industry_specs[ORIGINAL_IRON_MINE].layouts[0];
	const uint16_t tile_base = SPOIL_RECEIVER_TILE_LOCAL_BASE + to_underlying(kind) * SPOIL_RECEIVER_TILES_PER_KIND;

	std::vector<IndustryGfx> originals;
	IndustryTileLayout layout;
	// Each distinct original tile of the art layout gets one native tile, in order of first appearance.
	for (const auto &cell : art) {
		auto found = std::ranges::find(originals, cell.gfx);
		if (found == originals.end()) {
			if (originals.size() >= SPOIL_RECEIVER_TILES_PER_KIND) return false;
			const uint16_t local = tile_base + static_cast<uint16_t>(originals.size());
			_industile_mngr.SetEntitySpec(MakeSpoilReceiverTile(cell.gfx, local, cargo, recipe.cargo));
			originals.push_back(cell.gfx);
			found = std::prev(originals.end());
		}
		const IndustryGfx gfx = _industile_mngr.GetID(tile_base + static_cast<uint16_t>(found - originals.begin()), grfid);
		if (gfx == INVALID_INDUSTRYTILE || gfx < NEW_INDUSTRYTILEOFFSET) return false;
		layout.push_back({cell.ti, gfx});
	}

	/* The accept-only power station is the behaviour template: no production, no closure, Temperate only. */
	IndustrySpec spec = _origin_industry_specs[ORIGINAL_POWER_STATION];
	spec.layouts = {std::move(layout)};
	std::fill(std::begin(spec.conflicting), std::end(spec.conflicting), IT_INVALID);
	spec.check_proc = IndustryCheck::None;
	spec.accepts_cargo.fill(INVALID_CARGO);
	spec.accepts_cargo[0] = cargo;
	for (auto &row : spec.input_cargo_multiplier) std::fill(std::begin(row), std::end(row), 0);
	spec.accepts_cargo_label.fill(CT_INVALID);
	spec.accepts_cargo_label[0] = recipe.cargo;
	spec.climate_availability = LandscapeTypes{LandscapeType::Temperate};
	spec.behaviour = {};
	spec.map_colour = _origin_industry_specs[earth ? ORIGINAL_COAL_MINE : ORIGINAL_IRON_MINE].map_colour;
	spec.name = earth ? STR_INDUSTRY_NAME_EARTH_RECLAMATION_SITE : STR_INDUSTRY_NAME_ROCK_CRUSHER;
	// Never generated with the map or at random: a company builds a receiver where its haul ends.
	std::fill(std::begin(spec.appear_ingame), std::end(spec.appear_ingame), 0);
	std::fill(std::begin(spec.appear_creation), std::end(spec.appear_creation), 0);
	spec.callback_mask = {};
	spec.random_sounds = {};
	spec.badges = {};
	spec.grf_prop = SubstituteGRFFileProps(ORIGINAL_POWER_STATION);
	spec.grf_prop.override_id = IT_INVALID;
	spec.grf_prop.local_id = SPOIL_RECEIVER_INDUSTRY_LOCAL_BASE + to_underlying(kind);
	spec.grf_prop.grfid = grfid;
	_industry_mngr.SetEntitySpec(std::move(spec));
	return GetSpoilReceiverIndustryType(kind) != IT_INVALID;
}

bool InitialiseSpoilReceiverDefinitions(const GRFConfigList &activated)
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	if (CheckMultiLayerContentIdentity(required, activated) != MultiLayerContentIdentityError::None) return false;
	for (SpoilReceiverKind kind : {SpoilReceiverKind::Earth, SpoilReceiverKind::Rock}) {
		if (!InstallSpoilReceiverDefinition(kind)) return false;
	}
	return true;
}

IndustryType GetSpoilReceiverIndustryType(SpoilReceiverKind kind)
{
	if (kind >= SpoilReceiverKind::End) return IT_INVALID;
	const IndustryType type = _industry_mngr.GetID(SPOIL_RECEIVER_INDUSTRY_LOCAL_BASE + to_underlying(kind), GetSpoilReceiverGrfID());
	return GetSpoilReceiverKind(type) == kind ? type : IT_INVALID;
}

SpoilReceiverKind GetSpoilReceiverKind(IndustryType type)
{
	if (type >= NUM_INDUSTRYTYPES) return SpoilReceiverKind::End;
	const IndustrySpec *spec = GetIndustrySpec(type);
	// Only an enabled native definition in the package's own scope is a receiver; a GRF-loaded industry never is.
	if (!spec->enabled || spec->grf_prop.HasGrfFile() || spec->grf_prop.grfid != GetSpoilReceiverGrfID()) return SpoilReceiverKind::End;
	const int kind = static_cast<int>(spec->grf_prop.local_id) - SPOIL_RECEIVER_INDUSTRY_LOCAL_BASE;
	if (kind < 0 || kind >= to_underlying(SpoilReceiverKind::End)) return SpoilReceiverKind::End;
	return static_cast<SpoilReceiverKind>(kind);
}

SpoilReceiverKind GetSpoilReceiverKindForCargo(CargoType cargo)
{
	if (!IsValidCargoType(cargo)) return SpoilReceiverKind::End;
	const CargoLabel label = CargoSpec::Get(cargo)->label;
	for (SpoilReceiverKind kind : {SpoilReceiverKind::Earth, SpoilReceiverKind::Rock}) {
		if (GetSpoilReceiverRecipe(kind).cargo == label) return kind;
	}
	return SpoilReceiverKind::End;
}

/**
 * @param industry Industry identity.
 * @return The mutable receiver state, or nullptr.
 */
static SpoilReceiver *FindSpoilReceiver(IndustryID industry)
{
	auto it = _spoil_receivers.find(industry);
	return it == _spoil_receivers.end() ? nullptr : &it->second;
}

const SpoilReceiver *GetSpoilReceiver(IndustryID industry)
{
	return FindSpoilReceiver(industry);
}

std::vector<SpoilReceiver> CaptureSpoilReceivers()
{
	std::vector<SpoilReceiver> receivers;
	for (const auto &[id, receiver] : _spoil_receivers) receivers.push_back(receiver);
	return receivers;
}

void RegisterSpoilReceiver(const Industry &industry)
{
	const SpoilReceiverKind kind = GetSpoilReceiverKind(industry.type);
	if (kind == SpoilReceiverKind::End) return;
	SpoilReceiver receiver;
	receiver.industry = industry.index;
	receiver.kind = kind;
	_spoil_receivers[industry.index] = receiver;
}

void UnregisterSpoilReceiver(IndustryID industry)
{
	_spoil_receivers.erase(industry);
}

void ResetSpoilReceiverState()
{
	_spoil_receivers.clear();
	_staged_spoil_receivers.reset();
	ResetSpoilReceiverLoadStaging();
}

/**
 * @param industry Receiver industry.
 * @param kind Its kind.
 * @return Its stock entry for the spoil, or nullptr when the industry does not accept it.
 */
static const Industry::AcceptedCargo *GetSpoilReceiverStock(const Industry &industry, SpoilReceiverKind kind)
{
	const auto it = industry.GetCargoAccepted(GetCargoTypeByLabel(GetSpoilReceiverRecipe(kind).cargo));
	return it == std::end(industry.accepted) ? nullptr : &*it;
}

uint GetSpoilReceiverRoom(const Industry &industry)
{
	const SpoilReceiver *receiver = FindSpoilReceiver(industry.index);
	if (receiver == nullptr) return 0;
	const auto &recipe = GetSpoilReceiverRecipe(receiver->kind);
	const auto *stock = GetSpoilReceiverStock(industry, receiver->kind);
	if (stock == nullptr) return 0;
	const uint stock_room = recipe.stock_cap_t > stock->waiting ? recipe.stock_cap_t - stock->waiting : 0;
	const uint interval_room = recipe.interval_intake_t > receiver->interval_intake ? recipe.interval_intake_t - receiver->interval_intake : 0;
	return std::min(stock_room, interval_room);
}

uint GetSpoilReceiverRoomAt(const Station &station, CargoType cargo)
{
	const SpoilReceiverKind kind = GetSpoilReceiverKindForCargo(cargo);
	if (kind == SpoilReceiverKind::End) return UINT_MAX;
	uint64_t room = 0;
	for (const auto &near : station.industries_near) {
		const Industry *industry = near.industry;
		// Only receivers of this spoil count, under the exclusive-supplier filter that DeliverGoodsToIndustry applies.
		if (GetSpoilReceiverKind(industry->type) != kind) continue;
		if (industry->exclusive_supplier != INVALID_OWNER && industry->exclusive_supplier != station.owner) continue;
		room += GetSpoilReceiverRoom(*industry);
	}
	return ClampTo<uint>(room);
}

uint AdmitSpoilToReceiver(Industry &industry, CargoType cargo, uint requested, CompanyID company)
{
	SpoilReceiver *receiver = FindSpoilReceiver(industry.index);
	if (receiver == nullptr || GetSpoilReceiverKindForCargo(cargo) != receiver->kind) return 0;
	const uint admitted = std::min(requested, GetSpoilReceiverRoom(industry));
	if (admitted == 0) return 0;
	const auto &recipe = GetSpoilReceiverRecipe(receiver->kind);
	// The fee is settled once, here, for exactly the admitted tonnes and at the mode current at intake.
	const Money per_t = receiver->mode == SpoilReceiverMode::SurplusDisposal ? recipe.surplus_fee_per_t : recipe.fee_per_t;
	const Money fee = per_t * static_cast<int64_t>(admitted);
	receiver->interval_intake += static_cast<uint16_t>(admitted);
	receiver->fees += static_cast<uint64_t>(fee.base());
	++receiver->revision;
	if (Company::IsValidID(company) && fee != 0) SubtractMoneyFromCompany(company, CommandCost(ExpensesType::Other, fee));
	return admitted;
}

void AdvanceSpoilReceiver(Industry &industry)
{
	SpoilReceiver *receiver = FindSpoilReceiver(industry.index);
	if (receiver == nullptr) return;
	if (++receiver->interval_ticks < SPOIL_RECEIVER_INTERVAL_TICKS) return;
	receiver->interval_ticks = 0;
	bool changed = receiver->interval_intake != 0;
	receiver->interval_intake = 0;
	const auto &recipe = GetSpoilReceiverRecipe(receiver->kind);
	const auto it = industry.GetCargoAccepted(GetCargoTypeByLabel(recipe.cargo));
	if (it != std::end(industry.accepted)) {
		const bool crushing = receiver->kind == SpoilReceiverKind::Rock && receiver->mode == SpoilReceiverMode::Process;
		uint batches = std::min<uint>(it->waiting / recipe.batch_t, recipe.interval_intake_t / recipe.batch_t);
		// Crushing never overfills the aggregate store: a full store pauses it and the Rock stock waits.
		if (crushing) batches = std::min<uint>(batches, (recipe.aggregate_cap_t - receiver->aggregate) / recipe.aggregate_per_batch_t);
		if (batches != 0) {
			const uint tonnes = batches * recipe.batch_t;
			it->waiting -= static_cast<uint16_t>(tonnes);
			if (crushing) {
				receiver->aggregate += static_cast<uint16_t>(batches * recipe.aggregate_per_batch_t);
				receiver->crushed_t += tonnes;
				receiver->aggregate_t += batches * recipe.aggregate_per_batch_t;
				receiver->residue_t += batches * recipe.residue_per_batch_t;
			} else {
				receiver->disposed_t += tonnes;
			}
			changed = true;
		}
	}
	if (!changed) return;
	++receiver->revision;
	SetWindowDirty(WindowClass::IndustryView, industry.index);
}

SpoilReceiverError SetSpoilReceiverMode(IndustryID industry, SpoilReceiverMode mode, bool execute)
{
	SpoilReceiver *receiver = FindSpoilReceiver(industry);
	const Industry *site = Industry::GetIfValid(industry);
	if (receiver == nullptr || site == nullptr) return SpoilReceiverError::InvalidIndustry;
	// Only the Rock crusher has a choice; Earth is always disposed.
	if (mode >= SpoilReceiverMode::End || (receiver->kind != SpoilReceiverKind::Rock && mode != SpoilReceiverMode::Process)) return SpoilReceiverError::InvalidMode;
	if (_current_company != OWNER_DEITY && site->founder != _current_company) return SpoilReceiverError::NotFounder;
	if (!execute || receiver->mode == mode) return SpoilReceiverError::None;
	receiver->mode = mode;
	++receiver->revision;
	SetWindowDirty(WindowClass::IndustryView, industry);
	return SpoilReceiverError::None;
}

CommandCost CmdSetSpoilReceiverMode(DoCommandFlags flags, IndustryID industry, SpoilReceiverMode mode)
{
	if (SetSpoilReceiverMode(industry, mode, flags.Test(DoCommandFlag::Execute)) != SpoilReceiverError::None) return CMD_ERROR;
	return CommandCost();
}

void StageLoadedSpoilReceivers(std::vector<SpoilReceiver> rows)
{
	_staged_spoil_receivers = std::move(rows);
}

/**
 * @param row Loaded row.
 * @param industry Its industry, already checked to be a receiver of the row's kind.
 * @return Whether every field is in range for that kind, including the industry's own stock.
 */
static bool IsLoadedSpoilReceiverValid(const SpoilReceiver &row, const Industry &industry)
{
	const auto &recipe = GetSpoilReceiverRecipe(row.kind);
	const auto *stock = GetSpoilReceiverStock(industry, row.kind);
	// The mode belongs to the kind, the timer and intake to one interval, and the stores to their caps.
	if (row.mode >= SpoilReceiverMode::End || (row.kind != SpoilReceiverKind::Rock && row.mode != SpoilReceiverMode::Process)) return false;
	if (row.interval_ticks >= SPOIL_RECEIVER_INTERVAL_TICKS || row.interval_intake > recipe.interval_intake_t || row.aggregate > recipe.aggregate_cap_t || row.revision == 0) return false;
	return stock != nullptr && stock->waiting <= recipe.stock_cap_t;
}

bool AcceptLoadedSpoilReceivers()
{
	if (!_staged_spoil_receivers.has_value()) {
		// Nothing was loaded: the live store stays, and every receiver industry must still have its row.
		for (const Industry *industry : Industry::Iterate()) {
			if (GetSpoilReceiverKind(industry->type) != SpoilReceiverKind::End && !_spoil_receivers.contains(industry->index)) return false;
		}
		return true;
	}
	std::vector<SpoilReceiver> rows = std::move(*_staged_spoil_receivers);
	_staged_spoil_receivers.reset();
	std::map<IndustryID, SpoilReceiver> accepted;
	// Every row names a live receiver industry of its own kind, once, with in-range fields.
	for (const auto &row : rows) {
		const Industry *industry = Industry::GetIfValid(row.industry);
		if (industry == nullptr || row.kind >= SpoilReceiverKind::End || GetSpoilReceiverKind(industry->type) != row.kind) return false;
		if (!IsLoadedSpoilReceiverValid(row, *industry) || !accepted.emplace(row.industry, row).second) return false;
	}
	// Every receiver industry has its row; none can run on invented defaults.
	for (const Industry *industry : Industry::Iterate()) {
		if (GetSpoilReceiverKind(industry->type) != SpoilReceiverKind::End && !accepted.contains(industry->index)) return false;
	}
	_spoil_receivers = std::move(accepted);
	return true;
}

/**
 * Append one value's explicit big-endian bytes to an FNV-1a diagnostic hash.
 * @param digest Accumulator.
 * @param value Field value.
 */
static void HashSpoilReceiverField(uint64_t &digest, uint64_t value)
{
	for (int shift = 56; shift >= 0; shift -= 8) {
		digest ^= static_cast<uint8_t>(value >> shift);
		digest *= 1099511628211ULL;
	}
}

uint64_t ComputeSpoilReceiverDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	HashSpoilReceiverField(digest, _spoil_receivers.size());
	// Industry ID order from the map; the stock comes from the industry, which owns it.
	for (const auto &[id, receiver] : _spoil_receivers) {
		const Industry *industry = Industry::GetIfValid(id);
		const auto *stock = industry == nullptr ? nullptr : GetSpoilReceiverStock(*industry, receiver.kind);
		for (uint64_t value : {uint64_t{id.base()}, uint64_t{to_underlying(receiver.kind)}, uint64_t{to_underlying(receiver.mode)}, uint64_t{receiver.interval_ticks}, uint64_t{receiver.interval_intake}, uint64_t{receiver.aggregate}, receiver.disposed_t, receiver.crushed_t, receiver.aggregate_t, receiver.residue_t, receiver.fees, receiver.revision, uint64_t{stock == nullptr ? 0u : stock->waiting}}) {
			HashSpoilReceiverField(digest, value);
		}
	}
	return digest;
}
