/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_industry_definitions.cpp Native mines/processors reuse IIDS/TIDS and the published geological/recipe owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Groups 9-10
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs13-graphics-and-asset-pipeline.md, mine.* and facility.* logical footprints
 */

#include "stdafx.h"
#include "multilayer_industry.h"
#include "layer_industry_processor.h"
#include "industrytype.h"
#include "newgrf_commons.h"
#include "multilayer_content_identity.h"
#include "table/strings.h"

#include "safeguards.h"

extern const IndustrySpec _origin_industry_specs[NEW_INDUSTRYOFFSET]; ///< Read-only original templates; their identity grants no geological capability.
static constexpr uint16_t MINE_TILE_LOCAL_BASE = 0x8100; ///< Reserved native TIDS scope, distinct from the industry's IIDS pool.
static constexpr uint16_t MINE_TILES_PER_KIND = 16; ///< Nine actual 3x3 tile variants fit each fixed package-local family.

/** @return Exact pinned package scope shared by the geological mine and content capability owners. */
static GrfID LayerMineDefinitionGrfID()
{
	return GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package.grfid;
}

/**
 * Create a native machinery tile with explicit empty acceptance and inert inherited Surface effects.
 * @param original Visible original art substitute until the logical mine family is authored and packed by r2mlvs13.
 * @param local Stable package-local TIDS identity for this resource and tile variant.
 * @return Native tile definition; art cannot create cargo or excavate geological cells.
 */
static IndustryTileSpec MakeLayerMineTile(IndustryGfx original, uint16_t local)
{
	IndustryTileSpec tile = *GetIndustryTileSpec(original);
	tile.accepts_cargo.fill(INVALID_CARGO);
	tile.acceptance.fill(0);
	tile.accepts_cargo_label.fill(CT_INVALID);
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
	tile.grf_prop.grfid = LayerMineDefinitionGrfID();
	return tile;
}

/**
 * Register one complete native mine definition with nine machinery cells and its frozen output meaning.
 * @param index Exact position in the content-owned ten-resource catalogue, used only for stable local identity.
 * @return Whether its entire layout and industry resolve in existing saved/new mapping slots.
 */
static bool InstallLayerMineDefinition(size_t index)
{
	static constexpr std::array<StringID, 10> names{STR_INDUSTRY_NAME_LAYER_COAL_MINE, STR_INDUSTRY_NAME_LAYER_IRON_MINE,
		STR_INDUSTRY_NAME_LAYER_COPPER_MINE, STR_INDUSTRY_NAME_LAYER_BAUXITE_MINE, STR_INDUSTRY_NAME_LAYER_SALT_MINE,
		STR_INDUSTRY_NAME_LAYER_LITHIUM_MINE, STR_INDUSTRY_NAME_LAYER_NICKEL_MINE, STR_INDUSTRY_NAME_LAYER_COBALT_MINE,
		STR_INDUSTRY_NAME_LAYER_RARE_MINE, STR_INDUSTRY_NAME_LAYER_URANIUM_MINE};
	const auto &mine = GetMultiLayerMineRecipes()[index];
	const CargoType cargo = GetCargoTypeByLabel(mine.resource);
	if (!IsValidCargoType(cargo)) return false;
	const GrfID grfid = LayerMineDefinitionGrfID();
	const IndustrySpec &original = _origin_industry_specs[0];
	const auto &art = original.layouts[2];
	if (art.empty()) return false;
	IndustryTileLayout layout;
	// The manifest declares nine per-tile variants; use that actual machinery rectangle, plus the owner's separate loading approach.
	for (uint8_t variant = 0; variant < 9; ++variant) {
		const uint16_t local = MINE_TILE_LOCAL_BASE + static_cast<uint16_t>(index) * MINE_TILES_PER_KIND + variant;
		_industile_mngr.SetEntitySpec(MakeLayerMineTile(art[variant % art.size()].gfx, local));
		const IndustryGfx gfx = _industile_mngr.GetID(local, grfid);
		if (gfx == INVALID_INDUSTRYTILE) return false;
		layout.push_back({{static_cast<int8_t>(variant % 3), static_cast<int8_t>(variant / 3)}, gfx});
	}
	IndustrySpec spec = original;
	spec.layouts = {std::move(layout)};
	std::fill(std::begin(spec.conflicting), std::end(spec.conflicting), IT_INVALID);
	spec.check_proc = IndustryCheck::None;
	spec.accepts_cargo.fill(INVALID_CARGO);
	spec.accepts_cargo_label.fill(CT_INVALID);
	spec.produced_cargo.fill(INVALID_CARGO);
	spec.produced_cargo[0] = cargo;
	spec.produced_cargo_label.fill(CT_INVALID);
	spec.produced_cargo_label[0] = mine.resource;
	std::fill(std::begin(spec.production_rate), std::end(spec.production_rate), 0);
	// No independent-input callback, inherited rate, random generation or closure may fabricate finite geological output.
	for (auto &row : spec.input_cargo_multiplier) std::fill(std::begin(row), std::end(row), 0);
	std::fill(std::begin(spec.appear_ingame), std::end(spec.appear_ingame), 0);
	std::fill(std::begin(spec.appear_creation), std::end(spec.appear_creation), 0);
	spec.climate_availability = LandscapeTypes{LandscapeType::Temperate};
	spec.behaviour = {};
	spec.callback_mask = {};
	spec.random_sounds = {};
	spec.badges = {};
	spec.name = names[index];
	spec.enabled = true;
	spec.grf_prop = SubstituteGRFFileProps(0);
	spec.grf_prop.override_id = IT_INVALID;
	spec.grf_prop.local_id = LAYER_MINE_LOCAL_BASE + static_cast<uint16_t>(index);
	spec.grf_prop.grfid = grfid;
	_industry_mngr.SetEntitySpec(std::move(spec));
	return _industry_mngr.GetID(LAYER_MINE_LOCAL_BASE + static_cast<uint16_t>(index), grfid) != IT_INVALID;
}

/** @copydoc InitialiseLayerMineDefinitions */
bool InitialiseLayerMineDefinitions(const GRFConfigList &activated)
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	if (CheckMultiLayerContentIdentity(required, activated) != MultiLayerContentIdentityError::None) return false;
	// Register definitions only; terrain, geological claims and reserve/output are untouched by catalogue installation.
	for (size_t index = 0; index < GetMultiLayerMineRecipes().size(); ++index) {
		if (!InstallLayerMineDefinition(index)) return false;
	}
	return true;
}

/** @copydoc GetLayerMineIndustryType */
IndustryType GetLayerMineIndustryType(CargoLabel resource)
{
	const auto recipes = GetMultiLayerMineRecipes();
	const auto record = std::ranges::find(recipes, resource, &MultiLayerMineRecipe::resource);
	if (record == recipes.end()) return IT_INVALID;
	const uint16_t local = LAYER_MINE_LOCAL_BASE + static_cast<uint16_t>(record - recipes.begin());
	const IndustryType type = _industry_mngr.GetID(local, LayerMineDefinitionGrfID());
	return IsLayerMineDefinition(type) ? type : IT_INVALID;
}

/** Published primary choices define the seven stable native Surface family identities; alternatives share their actual inventory. */
static constexpr std::array<MultiLayerRecipeID, 7> PROCESSOR_PRIMARY{MultiLayerRecipeID{"CFMT"}, MultiLayerRecipeID{"CHEM"},
	MultiLayerRecipeID{"BATT"}, MultiLayerRecipeID{"ELEC"}, MultiLayerRecipeID{"COMP"}, MultiLayerRecipeID{"RBOT"}, MultiLayerRecipeID{"MKIT"}};

/**
 * Install one Surface facility with the exact union of all its content-owned alternatives' input/output meanings.
 * @param index Stable position in the seven-family primary catalogue.
 * @return Whether all sixteen machinery variants and the industry's IIDS identity resolve.
 */
static bool InstallLayerProcessorDefinition(size_t index)
{
	static constexpr std::array<StringID, 7> names{STR_INDUSTRY_NAME_LAYER_METAL_REFINERY, STR_INDUSTRY_NAME_LAYER_CHEMICAL_WORKS,
		STR_INDUSTRY_NAME_LAYER_BATTERY_WORKS, STR_INDUSTRY_NAME_LAYER_ELECTRONICS_WORKS, STR_INDUSTRY_NAME_LAYER_COMPONENT_WORKS,
		STR_INDUSTRY_NAME_LAYER_ROBOTICS_WORKS, STR_INDUSTRY_NAME_LAYER_MIRROR_WORKS};
	const auto records = GetMultiLayerManufacturingRecipes();
	const auto primary = std::ranges::find(records, PROCESSOR_PRIMARY[index], [](const auto &item) { return item.recipe.id; });
	if (primary == records.end()) return false;
	CargoTypes inputs, outputs;
	// Alternatives form one family: retain every deliverable input and product slot without running multiple recipes at once.
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
	IndustrySpec spec = _origin_industry_specs[1];
	spec.accepts_cargo.fill(INVALID_CARGO);
	spec.produced_cargo.fill(INVALID_CARGO);
	size_t slot = 0;
	for (CargoType cargo : inputs) spec.accepts_cargo[slot++] = cargo;
	slot = 0;
	for (CargoType cargo : outputs) spec.produced_cargo[slot++] = cargo;
	spec.accepts_cargo_label.fill(CT_INVALID);
	spec.produced_cargo_label.fill(CT_INVALID);
	IndustryTileLayout layout;
	const auto &art = _origin_industry_specs[6].layouts.front();
	if (art.empty()) return false;
	// The public facility family has a 4x4 per-tile footprint; every tile explicitly accepts its actual declared input union.
	for (uint8_t variant = 0; variant < 16; ++variant) {
		const uint16_t local = LAYER_PROCESSOR_LOCAL_BASE + static_cast<uint16_t>(index) * 16 + variant;
		IndustryTileSpec tile = MakeLayerMineTile(art[variant % art.size()].gfx, local);
		size_t input_slot = 0;
		for (CargoType cargo : inputs) {
			tile.accepts_cargo[input_slot] = cargo;
			// Native installation follows GRF polishing: use the sixteen runtime slots, not the three-label original-industry translation array.
			tile.acceptance[input_slot++] = 8;
		}
		_industile_mngr.SetEntitySpec(std::move(tile));
		const IndustryGfx gfx = _industile_mngr.GetID(local, LayerMineDefinitionGrfID());
		if (gfx == INVALID_INDUSTRYTILE) return false;
		layout.push_back({{static_cast<int8_t>(variant % 4), static_cast<int8_t>(variant / 4)}, gfx});
	}
	spec.layouts = {std::move(layout)};
	std::fill(std::begin(spec.conflicting), std::end(spec.conflicting), IT_INVALID);
	spec.check_proc = IndustryCheck::None;
	std::fill(std::begin(spec.production_rate), std::end(spec.production_rate), 0);
	for (auto &row : spec.input_cargo_multiplier) std::fill(std::begin(row), std::end(row), 0);
	std::fill(std::begin(spec.appear_ingame), std::end(spec.appear_ingame), 0);
	std::fill(std::begin(spec.appear_creation), std::end(spec.appear_creation), 0);
	spec.climate_availability = LandscapeTypes{LandscapeType::Temperate};
	spec.behaviour = {};
	spec.callback_mask = {};
	spec.random_sounds = {};
	spec.badges = {};
	spec.name = names[index];
	spec.enabled = true;
	spec.grf_prop = SubstituteGRFFileProps(1);
	spec.grf_prop.override_id = IT_INVALID;
	spec.grf_prop.local_id = LAYER_PROCESSOR_LOCAL_BASE + static_cast<uint16_t>(index);
	spec.grf_prop.grfid = LayerMineDefinitionGrfID();
	_industry_mngr.SetEntitySpec(std::move(spec));
	return _industry_mngr.GetID(LAYER_PROCESSOR_LOCAL_BASE + static_cast<uint16_t>(index), LayerMineDefinitionGrfID()) != IT_INVALID;
}

/** @copydoc InitialiseLayerProcessorDefinitions */
bool InitialiseLayerProcessorDefinitions(const GRFConfigList &activated)
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	if (CheckMultiLayerContentIdentity(required, activated) != MultiLayerContentIdentityError::None) return false;
	// Publish complete Surface definitions; recipe stock, work and choice are created only with a real Industry instance.
	for (size_t index = 0; index < PROCESSOR_PRIMARY.size(); ++index) {
		if (!InstallLayerProcessorDefinition(index)) return false;
	}
	return true;
}

/** @copydoc GetLayerProcessorIndustryType */
IndustryType GetLayerProcessorIndustryType(MultiLayerRecipeID primary)
{
	const auto entry = std::ranges::find(PROCESSOR_PRIMARY, primary);
	if (entry == PROCESSOR_PRIMARY.end()) return IT_INVALID;
	const uint16_t local = LAYER_PROCESSOR_LOCAL_BASE + static_cast<uint16_t>(entry - PROCESSOR_PRIMARY.begin());
	const IndustryType type = _industry_mngr.GetID(local, LayerMineDefinitionGrfID());
	return IsLayerProcessorDefinition(type) ? type : IT_INVALID;
}
