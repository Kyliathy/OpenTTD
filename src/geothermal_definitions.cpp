/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_definitions.cpp Canonical first collector IIDS/TIDS and explicit Deep input-only capability before paid native construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group1B
 */

#include "stdafx.h"
#include "geothermal.h"
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

/** @copydoc IsGeothermalCollectorDefinition */
bool IsGeothermalCollectorDefinition(IndustryType type)
{
	if (type >= NUM_INDUSTRYTYPES) return false;
	const auto *spec = GetIndustrySpec(type);
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector);
	// Exact package/local identity, inert legacy production and the frozen
	// single complete layout precede capability; a translated name or art mask
	// cannot grant another definition this collector's construction/heat rights.
	if (!spec->enabled || spec->grf_prop.HasGrfFile() || spec->grf_prop.grfid != GeothermalDefinitionGrfID() ||
		spec->grf_prop.local_id != GEOTHERMAL_COLLECTOR_LOCAL_ID || spec->callback_mask.Any() || spec->behaviour.Any() ||
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
	// bounded3x3 position; missing/reordered/foreign tile definitions refuse.
	for (size_t index = 0; index < spec->layouts.front().size(); ++index) {
		const auto &cell = spec->layouts.front()[index];
		if (cell.ti.x != index % terms->width || cell.ti.y != index / terms->width || cell.gfx >= NUM_INDUSTRYTILES) return false;
		const auto *tile = GetIndustryTileSpec(cell.gfx);
		if (!tile->enabled || tile->grf_prop.HasGrfFile() || tile->grf_prop.grfid != GeothermalDefinitionGrfID() ||
			tile->grf_prop.local_id != GEOTHERMAL_COLLECTOR_LOCAL_ID + index || tile->callback_mask.Any()) return false;
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
		if (tile_inputs != expected || std::ranges::count_if(tile->accepts_cargo, IsValidCargoType) != terms->inputs.size()) return false; // Every machinery cell accepts precisely the frozen three inputs once.
	}
	return true;
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

/** @copydoc InitialiseGeothermalCollectorDefinitions */
bool InitialiseGeothermalCollectorDefinitions(const GRFConfigList &activated)
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	if (CheckMultiLayerContentIdentity(required, activated) != MultiLayerContentIdentityError::None) return false;
	const auto *terms = GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector);
	std::array<uint16_t, 9> tile_locals;
	for (size_t index = 0; index < tile_locals.size(); ++index) tile_locals[index] = GEOTHERMAL_COLLECTOR_LOCAL_ID + static_cast<uint16_t>(index);
	// Both canonical tables must hold the complete facility before installing
	// its first tile; capacity estimates allocate neither mapping nor machinery.
	if (!_industry_mngr.CanAllocateEntityID(GEOTHERMAL_COLLECTOR_LOCAL_ID, GeothermalDefinitionGrfID()) ||
		!HasGeothermalMappingCapacity(_industile_mngr, tile_locals, INVALID_INDUSTRYTILE)) return false;
	const auto existing = _industry_mngr.GetID(GEOTHERMAL_COLLECTOR_LOCAL_ID, GeothermalDefinitionGrfID());
	// A saved unpolished mapping may restore into disabled specs. An enabled
	// foreign/malformed occupant must refuse before any of its shared slots change.
	if (existing != IT_INVALID && GetIndustrySpec(existing)->enabled && !IsGeothermalCollectorDefinition(existing)) return false;
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
		tile.grf_prop.grfid = GeothermalDefinitionGrfID(); tile.grf_prop.local_id = GEOTHERMAL_COLLECTOR_LOCAL_ID + index;
		_industile_mngr.SetEntitySpec(std::move(tile));
		const auto gfx = _industile_mngr.GetID(GEOTHERMAL_COLLECTOR_LOCAL_ID + index, GeothermalDefinitionGrfID());
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
	spec.name = STR_INDUSTRY_NAME_GEOTHERMAL_COLLECTOR;
	spec.grf_prop = SubstituteGRFFileProps(0); spec.grf_prop.override_id = IT_INVALID;
	spec.grf_prop.grfid = GeothermalDefinitionGrfID(); spec.grf_prop.local_id = GEOTHERMAL_COLLECTOR_LOCAL_ID;
	_industry_mngr.SetEntitySpec(std::move(spec));
	return GetGeothermalCollectorIndustryType() != IT_INVALID;
}
