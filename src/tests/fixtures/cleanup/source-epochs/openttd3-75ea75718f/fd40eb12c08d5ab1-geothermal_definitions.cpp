/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_definitions.cpp Canonical collector/riser/powerhouse IIDS/TIDS and explicit input-only capability before paid native construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups1B/5B/8C/8E
 */

#include "stdafx.h"
#include "geothermal.h"
#include "geothermal_riser.h"
#include "geothermal_generator.h"
#include "multilayer_industry.h"
#include "multilayer_content_identity.h"
#include "industrytype.h"
#include "newgrf_commons.h"
#include "table/strings.h"
#include "safeguards.h"

extern const IndustrySpec _origin_industry_specs[NEW_INDUSTRYOFFSET]; ///< Original templates supply provisional native display only; they grant no heat or construction authority.

/** @return Exact immutable native package scope; the existing IIDS/TIDS managers own all runtime identities. */
static GrfID GeothermalDefinitionGrfID()
{
	return GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content->package.grfid;
}

/** @param manager Actual canonical IIDS/TIDS allocator. @param locals Complete requested native local identity set. @param invalid Its actual missing-ID sentinel. @return Whether all missing identities fit before any definition write, preserving existing saved slots. */
static bool HasGeothermalMappingCapacity(const OverrideManagerBase &manager, std::span<const uint16_t> locals, uint16_t invalid)
{
	size_t needed = 0, free = 0;
	// The existing allocator reuses native saved mappings before taking empty
	// extension slots. Preflight that same public capacity for the entire family
	// so a full table cannot leave partial collector machinery registered.
	for (const auto local : locals) needed += manager.GetID(local, GeothermalDefinitionGrfID()) == invalid;
	for (uint16_t index = manager.GetMaxOffset(); index < manager.GetMaxMapping(); ++index) {
		const auto &mapping = manager.mappings[index];
		if (mapping.entity_id == 0 && mapping.grfid.Empty()) ++free;
	}
	return free >= needed;
}

/** @param type Native definition to validate. @param kind Frozen construction family. @param local_id Exact native family identity. @return Whether the whole native layout and stock contract match without changing any mapping. */
static bool IsGeothermalConstructionDefinition(IndustryType type, MultiLayerGeothermalConstructionKind kind, uint16_t local_id)
{
	if (type >= NUM_INDUSTRYTYPES) return false;
	const auto *spec = GetIndustrySpec(type);
	const auto *terms = GetMultiLayerGeothermalConstruction(kind);
	// Exact package/local identity, inert legacy production and the frozen
	// single complete layout precede capability; a translated name or art mask
	// cannot grant another definition this facility's construction rights.
	if (!spec->enabled || spec->grf_prop.HasGrfFile() || spec->grf_prop.grfid != GeothermalDefinitionGrfID() ||
		spec->grf_prop.local_id != local_id || spec->callback_mask.Any() || spec->behaviour.Any() ||
		spec->layouts.size() != 1 || spec->layouts.front().size() != terms->width * terms->height ||
		std::ranges::any_of(spec->produced_cargo, IsValidCargoType)) return false;
	CargoTypes expected, actual;
	for (const auto &input : terms->inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		if (!IsValidCargoType(cargo)) return false;
		expected.Set(cargo);
	}
	for (const auto cargo : spec->accepts_cargo) {
		if (IsValidCargoType(cargo)) actual.Set(cargo);
	}
	if (actual != expected || std::ranges::count_if(spec->accepts_cargo, IsValidCargoType) != terms->inputs.size()) return false; // Duplicate/extra stock slots cannot hide behind a collapsed cargo mask.
	// Every native machinery cell belongs to this exact TIDS family and
	// bounded complete position; missing/reordered/foreign tile definitions refuse.
	for (size_t index = 0; index < spec->layouts.front().size(); ++index) {
		const auto &cell = spec->layouts.front()[index];
		if (cell.ti.x != index % terms->width || cell.ti.y != index / terms->width || cell.gfx >= NUM_INDUSTRYTILES) return false;
		const auto *tile = GetIndustryTileSpec(cell.gfx);
		if (!tile->enabled || tile->grf_prop.HasGrfFile() || tile->grf_prop.grfid != GeothermalDefinitionGrfID() ||
			tile->grf_prop.local_id != local_id + index || tile->callback_mask.Any()) return false;
		CargoTypes tile_inputs;
		// Actual per-tile acceptance must agree with the one native inventory;
		// a missing/duplicate meaning or mutable multiplier cannot create another
		// invisible construction-input path under an otherwise valid industry ID.
		for (size_t slot = 0; slot < tile->accepts_cargo.size(); ++slot) {
			const auto cargo = tile->accepts_cargo[slot];
			if (!IsValidCargoType(cargo)) continue;
			if (tile->acceptance[slot] != 8) return false;
			tile_inputs.Set(cargo);
		}
		if (tile_inputs != expected || std::ranges::count_if(tile->accepts_cargo, IsValidCargoType) != terms->inputs.size()) return false; // Every machinery cell accepts precisely the frozen inputs once.
	}
	return true;
}

/** @copydoc IsGeothermalCollectorDefinition */
bool IsGeothermalCollectorDefinition(IndustryType type)
{
	return IsGeothermalConstructionDefinition(type, MultiLayerGeothermalConstructionKind::Collector, GEOTHERMAL_COLLECTOR_LOCAL_ID);
}

/** @copydoc IsGeothermalRiserDefinition */
bool IsGeothermalRiserDefinition(IndustryType type)
{
	return IsGeothermalConstructionDefinition(type, MultiLayerGeothermalConstructionKind::Riser, GEOTHERMAL_RISER_LOCAL_ID);
}

/** @copydoc GetGeothermalCollectorIndustryType */
IndustryType GetGeothermalCollectorIndustryType()
{
	const auto type = _industry_mngr.GetID(GEOTHERMAL_COLLECTOR_LOCAL_ID, GeothermalDefinitionGrfID());
	return IsGeothermalCollectorDefinition(type) ? type : IT_INVALID;
}

/** @copydoc GetGeothermalCollectorIndustryCapability */
const LayerIndustryCapability *GetGeothermalCollectorIndustryCapability(IndustryType type)
{
	if (!IsGeothermalCollectorDefinition(type)) return nullptr;
	static const LayerIndustryCapability capability{{MapLayer::DeepUnderground}, CT_INVALID, {},
		GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->introduction_year, 0, 1, "facility.vent-collector"};
	return &capability;
}

/** @param activated Exact admitted package. @param kind Frozen construction family. @param local_id Reserved native industry/tile family. @param name Localized facility name. @param execute False validates the complete family's actual shared capacity/occupants without any mapping/spec write. @return Complete canonical definition registration or unchanged preflight refusal; no actual facility is funded. */
static bool InitialiseGeothermalConstructionDefinition(const GRFConfigList &activated, MultiLayerGeothermalConstructionKind kind, uint16_t local_id, StringID name, bool execute = true)
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	if (CheckMultiLayerContentIdentity(required, activated) != MultiLayerContentIdentityError::None) return false;
	const auto *terms = GetMultiLayerGeothermalConstruction(kind);
	std::vector<uint16_t> tile_locals(terms->width * terms->height);
	for (size_t index = 0; index < tile_locals.size(); ++index) tile_locals[index] = local_id + static_cast<uint16_t>(index);
	// Both canonical tables must hold the complete facility before installing
	// its first tile; capacity estimates allocate neither mapping nor machinery.
	if (!_industry_mngr.CanAllocateEntityID(local_id, GeothermalDefinitionGrfID()) ||
		!HasGeothermalMappingCapacity(_industile_mngr, tile_locals, INVALID_INDUSTRYTILE)) return false;
	const auto existing = _industry_mngr.GetID(local_id, GeothermalDefinitionGrfID());
	// A saved unpolished mapping may restore into disabled specs. An enabled
	// foreign/malformed occupant must refuse before any of its shared slots change.
	if (existing != IT_INVALID && GetIndustrySpec(existing)->enabled && !IsGeothermalConstructionDefinition(existing, kind, local_id)) return false;
	CargoTypes inputs;
	// Resolve the entire frozen delivered-input contract before the first
	// definition write; a missing cargo cannot publish a partial collector.
	for (const auto &input : terms->inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		if (!IsValidCargoType(cargo)) return false;
		inputs.Set(cargo);
	}
	// Even an orphaned saved TIDS mapping may point at a live foreign tile.
	// Resolve every existing enabled occupant before writing the first cell;
	// restoration into disabled specs is permitted, replacing another owner is not.
	for (size_t index = 0; index < tile_locals.size(); ++index) {
		const auto gfx = _industile_mngr.GetID(tile_locals[index], GeothermalDefinitionGrfID());
		if (gfx == INVALID_INDUSTRYTILE) continue;
		const auto *tile = GetIndustryTileSpec(gfx);
		if (tile->enabled && (tile->grf_prop.HasGrfFile() || tile->grf_prop.grfid != GeothermalDefinitionGrfID() || tile->grf_prop.local_id != tile_locals[index])) return false; // Canonical mappings do not grant permission to replace an enabled foreign definition.
	}
	const auto &original = _origin_industry_specs[0];
	if (original.layouts.empty()) return false;
	const auto &art = original.layouts.front();
	if (art.empty()) return false;
	if (!execute) return true;
	IndustryTileLayout layout;
	// Native tile definitions expose exactly the real accepted input meanings;
	// inherited Surface smoke/farms/production and automatic graphics effects
	// remain inert while the actual paid whole-facility participant owns completion.
	for (uint8_t index = 0; index < terms->width * terms->height; ++index) {
		IndustryTileSpec tile = *GetIndustryTileSpec(art[index % art.size()].gfx);
		tile.accepts_cargo.fill(INVALID_CARGO); tile.acceptance.fill(0); tile.accepts_cargo_label.fill(CT_INVALID);
		size_t slot = 0;
		for (const auto cargo : inputs) { tile.accepts_cargo[slot] = cargo; tile.acceptance[slot++] = 8; }
		tile.anim_production = INDUSTRYTILE_NOANIM; tile.anim_next = INDUSTRYTILE_NOANIM; tile.anim_state = false;
		tile.callback_mask = {}; tile.animation = {}; tile.special_flags = {}; tile.enabled = true;
		tile.grf_prop = SubstituteGRFFileProps(art[index % art.size()].gfx); tile.grf_prop.override_id = INVALID_INDUSTRYTILE;
		tile.grf_prop.grfid = GeothermalDefinitionGrfID(); tile.grf_prop.local_id = local_id + index;
		_industile_mngr.SetEntitySpec(std::move(tile));
		const auto gfx = _industile_mngr.GetID(local_id + index, GeothermalDefinitionGrfID());
		if (gfx == INVALID_INDUSTRYTILE) return false;
		layout.push_back({{static_cast<int8_t>(index % terms->width), static_cast<int8_t>(index / terms->width)}, gfx});
	}
	IndustrySpec spec = original; spec.layouts = {std::move(layout)};
	spec.accepts_cargo.fill(INVALID_CARGO); spec.accepts_cargo_label.fill(CT_INVALID);
	size_t slot = 0; for (const auto cargo : inputs) spec.accepts_cargo[slot++] = cargo;
	spec.produced_cargo.fill(INVALID_CARGO); spec.produced_cargo_label.fill(CT_INVALID);
	std::fill(std::begin(spec.conflicting), std::end(spec.conflicting), IT_INVALID);
	std::fill(std::begin(spec.production_rate), std::end(spec.production_rate), 0);
	for (auto &row : spec.input_cargo_multiplier) std::fill(std::begin(row), std::end(row), 0);
	std::fill(std::begin(spec.appear_ingame), std::end(spec.appear_ingame), 0);
	std::fill(std::begin(spec.appear_creation), std::end(spec.appear_creation), 0);
	spec.check_proc = IndustryCheck::None; spec.climate_availability = LandscapeTypes{LandscapeType::Temperate};
	spec.behaviour = {}; spec.callback_mask = {}; spec.random_sounds = {}; spec.badges = {}; spec.enabled = true;
	spec.name = name;
	spec.grf_prop = SubstituteGRFFileProps(0); spec.grf_prop.override_id = IT_INVALID;
	spec.grf_prop.grfid = GeothermalDefinitionGrfID(); spec.grf_prop.local_id = local_id;
	_industry_mngr.SetEntitySpec(std::move(spec));
	return IsGeothermalConstructionDefinition(_industry_mngr.GetID(local_id, GeothermalDefinitionGrfID()), kind, local_id);
}

/** @copydoc InitialiseGeothermalCollectorDefinitions */
bool InitialiseGeothermalCollectorDefinitions(const GRFConfigList &activated)
{
	return InitialiseGeothermalConstructionDefinition(activated, MultiLayerGeothermalConstructionKind::Collector, GEOTHERMAL_COLLECTOR_LOCAL_ID, STR_INDUSTRY_NAME_GEOTHERMAL_COLLECTOR);
}

/** @copydoc InitialiseGeothermalRiserDefinitions */
bool InitialiseGeothermalRiserDefinitions(const GRFConfigList &activated)
{
	return InitialiseGeothermalConstructionDefinition(activated, MultiLayerGeothermalConstructionKind::Riser, GEOTHERMAL_RISER_LOCAL_ID, STR_INDUSTRY_NAME_GEOTHERMAL_RISER);
}

/** @copydoc InitialiseGeothermalGeneratorDefinitions */
bool InitialiseGeothermalGeneratorDefinitions(const GRFConfigList &activated)
{
	const std::array<uint16_t, 2> locals{GEOTHERMAL_POWERHOUSE_LOCAL_ID, GEOTHERMAL_MAGMA_LOCAL_ID};
	const std::array models{GeothermalGeneratorModel::VentTap, GeothermalGeneratorModel::MagmaLoop};
	std::vector<uint16_t> tiles;
	// Both complete dated families must fit together. Independent one-family
	// checks would wrongly accept the same final free slot twice and leave a
	// partial registration or overwrite saved mappings after the first write.
	for (size_t index = 0; index < locals.size(); ++index) {
		const auto *operating = GetMultiLayerGeothermalGeneratorTerms(models[index]);
		if (operating == nullptr) return false;
		const auto *terms = GetMultiLayerGeothermalConstruction(operating->construction);
		const auto geometry = GetGeothermalGeneratorGeometry(models[index]);
		// Changed content cannot stretch already frozen source geometry or let
		// a smaller combined capacity estimate authorize a partial larger family.
		if (terms == nullptr || !geometry.has_value() || terms->width != geometry->width || terms->height != geometry->height) return false;
		for (uint16_t position = 0; position < terms->width * terms->height; ++position) tiles.push_back(locals[index] + position);
	}
	if (!HasGeothermalMappingCapacity(_industry_mngr, locals, IT_INVALID) || !HasGeothermalMappingCapacity(_industile_mngr, tiles, INVALID_INDUSTRYTILE) ||
		!InitialiseGeothermalConstructionDefinition(activated, MultiLayerGeothermalConstructionKind::Powerhouse, GEOTHERMAL_POWERHOUSE_LOCAL_ID, STR_INDUSTRY_NAME_GEOTHERMAL_POWERHOUSE, false) ||
		!InitialiseGeothermalConstructionDefinition(activated, MultiLayerGeothermalConstructionKind::MagmaPowerhouse, GEOTHERMAL_MAGMA_LOCAL_ID, STR_INDUSTRY_NAME_GEOTHERMAL_MAGMA, false)) return false;
	return InitialiseGeothermalConstructionDefinition(activated, MultiLayerGeothermalConstructionKind::Powerhouse, GEOTHERMAL_POWERHOUSE_LOCAL_ID, STR_INDUSTRY_NAME_GEOTHERMAL_POWERHOUSE) &&
		InitialiseGeothermalConstructionDefinition(activated, MultiLayerGeothermalConstructionKind::MagmaPowerhouse, GEOTHERMAL_MAGMA_LOCAL_ID, STR_INDUSTRY_NAME_GEOTHERMAL_MAGMA);
}

/** @copydoc IsGeothermalGeneratorDefinition */
bool IsGeothermalGeneratorDefinition(IndustryType type)
{
	// The two original IIDS/TIDS families retain distinct immutable identities;
	// a generic industry or a first-model mapping cannot impersonate the other.
	return IsGeothermalConstructionDefinition(type, MultiLayerGeothermalConstructionKind::Powerhouse, GEOTHERMAL_POWERHOUSE_LOCAL_ID) ||
		IsGeothermalConstructionDefinition(type, MultiLayerGeothermalConstructionKind::MagmaPowerhouse, GEOTHERMAL_MAGMA_LOCAL_ID);
}

/** @copydoc GetGeothermalGeneratorIndustryType */
IndustryType GetGeothermalGeneratorIndustryType(GeothermalGeneratorModel model)
{
	const auto *terms = GetMultiLayerGeothermalGeneratorTerms(model);
	if (terms == nullptr) return IT_INVALID;
	const auto local = model == GeothermalGeneratorModel::VentTap ? GEOTHERMAL_POWERHOUSE_LOCAL_ID : GEOTHERMAL_MAGMA_LOCAL_ID;
	const auto type = _industry_mngr.GetID(local, GeothermalDefinitionGrfID());
	return IsGeothermalConstructionDefinition(type, terms->construction, local) ? type : IT_INVALID;
}

/** @copydoc GetGeothermalGeneratorIndustryCapability */
const LayerIndustryCapability *GetGeothermalGeneratorIndustryCapability(IndustryType type)
{
	if (!IsGeothermalGeneratorDefinition(type)) return nullptr;
	static const LayerIndustryCapability capability{{MapLayer::DeepUnderground}, CT_INVALID, {},
		GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Powerhouse)->introduction_year, 0, 1, "facility.vent-powerhouse"};
	static const LayerIndustryCapability magma{{MapLayer::DeepUnderground}, CT_INVALID, {},
		GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::MagmaPowerhouse)->introduction_year, 0, 1, "facility.magma-powerhouse"};
	return type == GetGeothermalGeneratorIndustryType(GeothermalGeneratorModel::MagmaLoop) ? &magma : &capability;
}

/** @copydoc GetGeothermalRiserIndustryType */
IndustryType GetGeothermalRiserIndustryType()
{
	const auto type = _industry_mngr.GetID(GEOTHERMAL_RISER_LOCAL_ID, GeothermalDefinitionGrfID());
	return IsGeothermalRiserDefinition(type) ? type : IT_INVALID;
}

/** @copydoc GetGeothermalRiserIndustryCapability */
const LayerIndustryCapability *GetGeothermalRiserIndustryCapability(IndustryType type)
{
	if (!IsGeothermalRiserDefinition(type)) return nullptr;
	static const LayerIndustryCapability capability{{MapLayer::Underground, MapLayer::DeepUnderground}, CT_INVALID, {},
		GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->introduction_year, 0, 1, "facility.utility-riser"};
	return &capability;
}
