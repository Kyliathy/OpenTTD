/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_industry_policy.cpp Whitelisted definition and complete excavated-site admission. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Groups 3-4; r2mlvs18-geothermal-energy-and-industry.md Group1B, exclusive collector capability. */

#include "stdafx.h"
#include "multilayer_industry.h"
#include "geothermal.h"
#include "geothermal_riser.h"
#include "geothermal_utility.h"
#include "industry.h"
#include "industrytype.h"
#include "map_layer.h"
#include "company_base.h"
#include "clear_map.h"
#include "landscape_cmd.h"
#include "command_func.h"
#include "bridge_map.h"
#include "vehicle_func.h"
#include "table/strings.h"

#include "safeguards.h"

/**
 * Read the profile's native industry identity scope, without maintaining another registry.
 * @return Exact first-party package GRFID owned by the content profile.
 */
static GrfID LayerIndustryGrfID()
{
	return GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package.grfid;
}

/**
 * Build immutable admission capabilities from the ten content-owned mine records.
 * @return Fixed catalogue ordered exactly as the published mine recipes; it owns no runtime identity or stock.
 */
static const std::array<LayerIndustryCapability, 10> &MineCapabilities()
{
	static const auto capabilities = []() {
		std::array<LayerIndustryCapability, 10> result{};
		const auto recipes = GetMultiLayerMineRecipes();
		static constexpr std::array<std::string_view, 10> assets{"mine.COAL", "mine.IORE", "mine.CORE", "mine.BAUX", "mine.SALT",
			"mine.LITH", "mine.NORE", "mine.COBT", "mine.RARE", "mine.URAN"};
		assert(recipes.size() == result.size());
		// Each published resource keeps its own layer/date/output bound; Deep equipment costs twice as much.
		for (size_t index = 0; index < recipes.size(); ++index) {
			const auto &mine = recipes[index];
			result[index] = {{mine.layer}, mine.resource, mine.recipe.id, mine.introduction_year,
				mine.output_capacity, static_cast<uint8_t>(mine.layer == MapLayer::DeepUnderground ? 2 : 1), assets[index]};
		}
		return result;
	}();
	return capabilities;
}

/**
 * Derive the seven Surface facility capabilities from their published primary recipes.
 * @return Fixed capability order shared with native IIDS installation; alternatives remain saved recipe choices.
 */
static const std::array<LayerIndustryCapability, 7> &ProcessorCapabilities()
{
	static const auto capabilities = []() {
		constexpr MultiLayerRecipeID primary[] = {MultiLayerRecipeID{"CFMT"}, MultiLayerRecipeID{"CHEM"},
			MultiLayerRecipeID{"BATT"}, MultiLayerRecipeID{"ELEC"}, MultiLayerRecipeID{"COMP"},
			MultiLayerRecipeID{"RBOT"}, MultiLayerRecipeID{"MKIT"}};
		std::array<LayerIndustryCapability, std::size(primary)> result{};
		const auto records = GetMultiLayerManufacturingRecipes();
		static constexpr std::array<std::string_view, 7> assets{"facility.metal-refinery", "facility.chemical-works", "facility.battery-works",
			"facility.electronics-works", "facility.component-works", "facility.robotics-works", "facility.mirror-kit-works"};
		// One facility identity covers its alternatives; output stock/work never belong to separately allocated recipe instances.
		for (size_t index = 0; index < result.size(); ++index) {
			const auto record = std::ranges::find_if(records, [index, &primary](const auto &item) { return item.recipe.id == primary[index]; });
			assert(record != records.end());
			result[index] = {{MapLayer::Surface}, CT_INVALID, record->recipe.id, record->introduction_year, record->output_capacity, 1, assets[index]};
		}
		return result;
	}();
	return capabilities;
}

/**
 * Match the exact transported input/output meanings of a native Surface facility, including all published alternatives.
 * @param spec Actual native industry definition, with no independent rate/callback production owner.
 * @param capability Facility's primary recipe capability.
 * @return Whether its accepted/produced slots are exactly the content-owned facility union.
 */
static bool MatchProcessorCargoes(const IndustrySpec &spec, const LayerIndustryCapability &capability)
{
	const auto records = GetMultiLayerManufacturingRecipes();
	const auto primary = std::ranges::find_if(records, [&capability](const auto &item) { return item.recipe.id == capability.recipe; });
	if (primary == records.end()) return false;
	CargoTypes inputs;
	CargoTypes outputs;
	// Resolve all alternatives by published facility/recipe meaning; no translated industry name or cargo-slot number is authority.
	for (const auto &record : records) {
		if (record.facility != primary->facility) continue;
		for (const auto &input : record.recipe.inputs) {
			const CargoType cargo = GetCargoTypeByLabel(input.label);
			if (!IsValidCargoType(cargo)) return false;
			inputs.Set(cargo);
		}
		for (const auto &output : record.recipe.outputs) {
			const CargoType cargo = GetCargoTypeByLabel(output.label);
			if (!IsValidCargoType(cargo)) return false;
			outputs.Set(cargo);
		}
	}
	CargoTypes actual_inputs;
	CargoTypes actual_outputs;
	// A duplicate/missing/extra slot cannot impersonate the declared all-input facility contract.
	for (CargoType cargo : spec.accepts_cargo) {
		if (!IsValidCargoType(cargo)) continue;
		if (actual_inputs.Test(cargo)) return false;
		actual_inputs.Set(cargo);
	}
	for (CargoType cargo : spec.produced_cargo) {
		if (!IsValidCargoType(cargo)) continue;
		if (actual_outputs.Test(cargo)) return false;
		actual_outputs.Set(cargo);
	}
	return inputs == actual_inputs && outputs == actual_outputs;
}

/** @copydoc GetLayerIndustryCapability */
const LayerIndustryCapability *GetLayerIndustryCapability(IndustryType type)
{
	if (type >= NUM_INDUSTRYTYPES) return nullptr;
	if (GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return nullptr;
	const auto *spec = GetIndustrySpec(type);
	// Only native definitions in the pinned package scope gain geology authority; GRF art/substitutes never do.
	if (!spec->enabled || spec->grf_prop.HasGrfFile() || spec->grf_prop.grfid != LayerIndustryGrfID()) return nullptr;
	const uint16_t local = spec->grf_prop.local_id;
	if (local == GEOTHERMAL_COLLECTOR_LOCAL_ID) return GetGeothermalCollectorIndustryCapability(type);
	if (local == GEOTHERMAL_RISER_LOCAL_ID) return GetGeothermalRiserIndustryCapability(type); // Exact input-only1980 machinery may use either rock layer, without inheriting vent or production authority.
	// Supported Surface processing stays explicit even though ordinary external industries also retain Surface permission.
	if (local >= LAYER_PROCESSOR_LOCAL_BASE && local < LAYER_PROCESSOR_LOCAL_BASE + ProcessorCapabilities().size()) {
		const auto &capability = ProcessorCapabilities()[local - LAYER_PROCESSOR_LOCAL_BASE];
		if (spec->callback_mask.Any()) return nullptr;
		return MatchProcessorCargoes(*spec, capability) ? &capability : nullptr;
	}
	// Reject unrelated native facilities before indexing the ten-record catalogue.
	if (local < LAYER_MINE_LOCAL_BASE || local >= LAYER_MINE_LOCAL_BASE + MineCapabilities().size()) return nullptr;
	const auto &capability = MineCapabilities()[local - LAYER_MINE_LOCAL_BASE];
	const CargoType cargo = GetCargoTypeByLabel(capability.resource);
	// Native extraction must have precisely its declared output and no alternative callback/input production owner.
	if (!IsValidCargoType(cargo) || spec->produced_cargo[0] != cargo || !spec->callback_mask.None() ||
			std::ranges::count_if(spec->produced_cargo, IsValidCargoType) != 1 ||
			std::ranges::any_of(spec->accepts_cargo, IsValidCargoType)) return nullptr;
	return &capability;
}

/** @copydoc IsLayerMineDefinition */
bool IsLayerMineDefinition(IndustryType type)
{
	const auto *capability = GetLayerIndustryCapability(type);
	return capability != nullptr && capability->resource != CT_INVALID;
}

/** @copydoc CheckLayerIndustryConstruction */
CommandCost CheckLayerIndustryConstruction(TileIndex tile, IndustryType type)
{
	const MapLayer layer = GetTileLayer(tile);
	if (!MultiLayerMap::IsValidLayer(layer)) return CMD_ERROR;
	const auto *capability = GetLayerIndustryCapability(type);
	if (capability == nullptr) {
		const auto *spec = type < NUM_INDUSTRYTYPES ? GetIndustrySpec(type) : nullptr;
		// A malformed or wrong-profile native mine/processor must never fall back to legacy Surface construction.
		if (spec != nullptr && spec->grf_prop.grfid == LayerIndustryGrfID() &&
				((spec->grf_prop.local_id >= LAYER_MINE_LOCAL_BASE && spec->grf_prop.local_id < LAYER_MINE_LOCAL_BASE + MineCapabilities().size()) ||
				(spec->grf_prop.local_id >= LAYER_PROCESSOR_LOCAL_BASE && spec->grf_prop.local_id < LAYER_PROCESSOR_LOCAL_BASE + ProcessorCapabilities().size()) ||
				spec->grf_prop.local_id == GEOTHERMAL_COLLECTOR_LOCAL_ID)) return CMD_ERROR;
		// Unadapted originals/GRFs retain Surface policy on every construction entry path.
		return layer == MapLayer::Surface ? CommandCost{} : CommandCost{STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER};
	}
	if (!capability->layers.Test(layer)) return CommandCost{STR_ERROR_NOT_ALLOWED_IN_MAP_LAYER};
	if (!IsLayerIndustryTechnologyAvailable(*capability, layer, GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) {
		return CommandCost{STR_ERROR_SITE_UNSUITABLE};
	}
	return CommandCost{};
}

/** @copydoc IsLayerIndustryTechnologyAvailable */
bool IsLayerIndustryTechnologyAvailable(const LayerIndustryCapability &capability, MapLayer layer, const MultiLayerProfileSelection &policy, TimerGameCalendar::Date date)
{
	// A scenario override may author infrastructure but cannot run this profile's Deep mineral technology before its calendar introduction.
	if (policy.id != MultiLayerGameplayProfileID::MultilayerIndustry || !capability.layers.Test(layer) ||
			TimerGameCalendar::ConvertDateToYMD(date).year < capability.introduction_year) return false;
	return layer != MapLayer::DeepUnderground || CheckDeepLayerAvailability(policy, date);
}

/** @copydoc ValidateMineDepositClaim */
LayerMineAdmission ValidateMineDepositClaim(const LayerMineRequest &request)
{
	LayerMineAdmission result;
	const auto *capability = GetLayerIndustryCapability(request.type);
	// Surface processors have an all-input inventory contract, never a finite seam claim.
	if (capability == nullptr || capability->resource == CT_INVALID) return result;
	if (!capability->layers.Test(GetTileLayer(request.tile))) {
		result.error = LayerIndustryError::WrongLayer;
		return result;
	}
	if (CheckLayerIndustryConstruction(request.tile, request.type).Failed()) {
		result.error = LayerIndustryError::Unavailable;
		return result;
	}
	if (!Company::IsValidID(request.company)) {
		result.error = LayerIndustryError::InvalidCompany;
		return result;
	}
	const auto &deposits = GetGeologyWorldState().deposits;
	const auto *deposit = deposits.Get(request.deposit);
	// A selected seam must describe this exact resource and rock layer; co-located deposits are independent.
	if (deposit == nullptr || deposit->resource != capability->resource || deposit->layer != GetTileLayer(request.tile)) {
		result.error = LayerIndustryError::InvalidDeposit;
		return result;
	}
	// Even an abandoned mine with no company claimant retains an exclusive physical binding until its empty footprint is removed.
	if (deposit->claimant != CompanyID::Invalid() || FindLayerMineIndustry(request.deposit) != IndustryID::Invalid()) {
		result.error = LayerIndustryError::Claimed;
		return result;
	}
	if (deposit->remaining_reserve == 0) {
		result.error = LayerIndustryError::Depleted;
		return result;
	}
	const auto *spec = GetIndustrySpec(request.type);
	if (request.layout >= spec->layouts.size()) return result;
	const auto &layout = spec->layouts[request.layout];
	// Bound all site work before allocating vectors; native machinery layouts cannot contain water-only check cells.
	if (layout.empty() || layout.size() > 64) return result;
	int max_y = 0;
	bool contact = false;
	// Every foundation is charged/checked once, and at least one actual machinery cell must touch the selected seam.
	for (const auto &cell : layout) {
		const TileIndex tile = TileAddWrap(request.tile, cell.ti.x, cell.ti.y);
		// Refuse plane-edge wraps, invalid offsets and repeated cells rather than constructing a different footprint.
		if (!IsValidTile(tile) || cell.gfx == GFX_WATERTILE_SPECIALCHECK ||
				std::ranges::find(result.machinery, tile) != result.machinery.end()) return result;
		result.machinery.push_back(tile);
		max_y = std::max(max_y, static_cast<int>(cell.ti.y));
		contact |= deposits.Find(tile) == request.deposit;
	}
	// This whole two-by-two approach remains outside the machinery and must be excavated before funding.
	for (int y = 1; y <= 2; ++y) {
		for (int x = 0; x < 2; ++x) result.loading.push_back(TileAddWrap(request.tile, x, max_y + y));
	}
	std::vector<TileIndex> footprint = result.machinery;
	footprint.insert(footprint.end(), result.loading.begin(), result.loading.end());
	const auto ground = CheckExcavatedFootprint(footprint);
	if (ground.error != ExcavationError::None) {
		result.error = LayerIndustryError::Footprint;
		result.failure_tile = ground.failure_tile;
		return result;
	}
	if (!contact) {
		result.error = LayerIndustryError::NoSeamContact;
		return result;
	}
	// Loading cells may already carry the company's conventional access, but another structure/owner cannot be displaced.
	for (TileIndex tile : result.loading) {
		const TileType kind = GetTileType(tile);
		if (kind == TileType::Clear) continue;
		// A rail/road approach is usable only by its actual owner; it never grants a free cross-layer route.
		if ((kind != TileType::Railway && kind != TileType::Road) || GetTileOwner(tile) != request.company) {
			result.error = LayerIndustryError::Access;
			result.failure_tile = tile;
			return result;
		}
	}
	result.cost = spec->GetConstructionCost() * capability->equipment_cost_factor;
	// Native foundations use already clear, flat cavity floor. No demolition/terraforming can run after claim admission.
	for (TileIndex tile : result.machinery) {
		// Funding cannot bury paid utilities under a new mine or undeclared
		// industry port; its owner must remove the cell explicitly first.
		if (!IsTileType(tile, TileType::Clear) || !IsTileFlat(tile) || IsBridgeAbove(tile) || EnsureNoVehicleOnGround(tile).Failed() || HasGeothermalUtilityCell(tile)) {
			result.error = LayerIndustryError::Occupied;
			result.failure_tile = tile;
			return result;
		}
		const auto cost = Command<Commands::LandscapeClear>::Do({DoCommandFlag::Auto, DoCommandFlag::NoTestTownRating, DoCommandFlag::NoModifyTownRating}, tile);
		if (cost.Failed()) {
			result.error = LayerIndustryError::Occupied;
			result.failure_tile = tile;
			return result;
		}
		result.cost += cost.GetCost();
	}
	if (!Industry::CanAllocateItem()) {
		result.error = LayerIndustryError::Capacity;
		return result;
	}
	result.expected_reserve = deposit->remaining_reserve;
	result.error = LayerIndustryError::None;
	return result;
}
