/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_model.cpp Immutable cutter tuning and actual unit-boundary validation. @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 1; docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, native Deep starter capacity */

#include "stdafx.h"
#include "tbm.h"
#include "train.h"

#include "safeguards.h"

static const TBMCutterModel _tbm_models[] = {
	{0, 64, 1800, 1, 1, 400000, 120000, 12000, 2, 3},
	{1, 48, 3200, 3, 2, 800000, 260000, 26000, 2, 3},
};

/** @copydoc GetDefaultTBMConsist */
TBMConsistRequest GetDefaultTBMConsist(MapLayer layer)
{
	switch (layer) {
		case MapLayer::Underground: return {2, 2};
		case MapLayer::DeepUnderground: return {2, 4};
		default: return {};
	}
}

/** @return Immutable initial tuning; measured corridor budgets remain a later acceptance obligation. */
std::span<const TBMCutterModel> GetTBMCutterModels()
{
	return _tbm_models;
}

/**
 * Resolve tuning without confusing logical IDs with runtime engine IDs.
 * @param model_id Profile-owned cutter design.
 * @return Matching definition, or nullptr for a wagon or unknown design.
 */
const TBMCutterModel *GetTBMCutterModel(uint16_t model_id)
{
	const auto models = GetTBMCutterModels();
	const auto it = std::ranges::find(models, model_id, &TBMCutterModel::model_id);
	return it == models.end() ? nullptr : &*it;
}

/**
 * Apply the shared profile's saved technology policy to an actual cutter operation.
 * @param model_id Profile-owned design.
 * @param layer Explicit world layer.
 * @param policy Saved profile and override policy.
 * @param date Synchronized calendar date.
 * @return Complete model/layer/date permission, without publishing gameplay state.
 */
bool IsTBMCutterAvailable(uint16_t model_id, MapLayer layer, const MultiLayerProfileSelection &policy, TimerGameCalendar::Date date)
{
	const auto *profile = GetMultiLayerContentProfile(policy.id);
	if (to_underlying(layer) >= to_underlying(MapLayer::End)) return false;
	// Cutter machinery belongs to the native profile; unknown versions and legacy worlds cannot acquire its capabilities.
	if (profile == nullptr || policy.id != MultiLayerGameplayProfileID::MultilayerIndustry || policy.legacy || policy.version != profile->version || GetTBMCutterModel(model_id) == nullptr) return false;
	const auto capabilities = GetMultiLayerVehicleCapabilities();
	const auto it = std::ranges::find(capabilities, model_id, &LayerVehicleCapability::model_id);
	// The profile owns physical layer and assembly permissions; physical tuning cannot redefine either restriction.
	if (it == capabilities.end() || !it->supported_layers.Test(layer) || it->commissioning != MultiLayerCommissioningFacility::RailPortalBay) return false;
	if (date < TimerGameCalendar::ConvertYMDToDate(it->introduction_year, 0, 1)) return false;
	// Even a Deep-rated design remains locked until the shared calendar policy permits actual Deep operations.
	if (layer == MapLayer::DeepUnderground && !CheckDeepLayerAvailability(policy, date)) return false;
	return true;
}

/**
 * Protect the articulated machinery while exposing only actual complete spoil wagons.
 * @param front Actual train chain head.
 * @param project Canonical fixed-part identities and owner.
 * @return All eligible wagon units in stable order, or an empty refused layout.
 */
TBMConsistLayout InspectTBMConsist(const Train &front, const TBMProject &project)
{
	TBMConsistLayout result;
	// Both identity and structural engine flags must match; an ordinary train cannot borrow another project's permanent layout.
	if (front.index != project.cutter || front.owner != project.company || !front.IsFrontEngine() || !front.IsEngine() || front.First() != &front || project.permanent_parts.size() < 2 || project.permanent_parts.front() != front.index || GetTBMCutterModel(project.model_id) == nullptr) return result;
	const Train *part = &front;
	// The immutable prefix must match every actual articulated part exactly; a missing/rearranged support refuses exchange.
	for (size_t index = 0; index < project.permanent_parts.size(); ++index) {
		if (part == nullptr) return result;
		// Fixed supports must belong to this cutter, carry no spoil and remain actual articulated parts rather than detachable wagons.
		if (part->index != project.permanent_parts[index] || part->owner != project.company || (index != 0 && !part->IsArticulatedPart()) || part->cargo_cap != 0 || part->cargo.TotalCount() != 0) return result;
		part = part->Next();
	}
	if (part != nullptr && part->IsArticulatedPart()) return result; // Undeclared fixed machinery cannot become an exchangeable tail.
	const Train *previous = &front;
	// Walk real unit boundaries so articulated/rear parts cannot be detached or counted as independent wagon capacity.
	for (Train *unit = front.GetNextUnit(); unit != nullptr; unit = unit->GetNextUnit()) {
		// Only dedicated same-owner wagon units with consistent backward boundaries participate in the real spoil journey.
		if (!unit->IsWagon() || unit->IsEngine() || unit->owner != project.company || unit->GetPrevUnit() != previous || !IsValidCargoType(unit->cargo_type)) return TBMConsistLayout{};
		const auto label = CargoSpec::Get(unit->cargo_type)->label;
		TBMConsistRole role;
		if (label == CargoLabel{"ERTH"}) {
			role = TBMConsistRole::EarthWagon;
		} else if (label == CargoLabel{"ROCK"}) {
			role = TBMConsistRole::RockWagon;
		} else {
			return TBMConsistLayout{};
		}
		// All cargo-bearing parts of an exchanged unit retain the head's dedicated spoil meaning and company.
		for (const Train *wagon_part = unit; wagon_part != unit->GetNextUnit(); wagon_part = wagon_part->Next()) {
			if (wagon_part == nullptr) return TBMConsistLayout{};
			if (wagon_part->owner != project.company || wagon_part->cargo_type != unit->cargo_type) return TBMConsistLayout{}; // Mixed cargo/ownership inside one articulated wagon cannot inherit the head's spoil permission.
		}
		result.exchangeable_units.push_back({unit->index, role});
		previous = unit;
	}
	result.error = TBMBlockedReason::None;
	return result;
}
