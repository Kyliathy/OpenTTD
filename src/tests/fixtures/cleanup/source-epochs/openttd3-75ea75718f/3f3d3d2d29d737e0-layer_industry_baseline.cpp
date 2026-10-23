/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_industry_baseline.cpp Bounded reuse of original Steel semantics with one physical cargo owner. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 11 */

#include "stdafx.h"
#include "layer_industry_baseline.h"
#include "multilayer_industry.h"
#include "industry.h"
#include "industrytype.h"
#include "openttd.h"

#include "safeguards.h"

/** @param spec Actual enabled definition. @param label Stable input meaning. @return Whether one actual input and no callbacks/foreign GRF own this baseline definition. */
static bool MatchBaselineInput(const IndustrySpec &spec, CargoLabel label)
{
	const CargoType cargo = GetCargoTypeByLabel(label);
	// The baseline adapter admits precisely the native single-input definition; arbitrary GRF callbacks cannot be replaced by this conversion owner.
	return GetCurrentMultiLayerProfileSelection().id == MultiLayerGameplayProfileID::MultilayerIndustry && spec.enabled &&
		!spec.grf_prop.HasGrfFile() && spec.callback_mask.None() && IsValidCargoType(cargo) &&
		std::ranges::count_if(spec.accepts_cargo, IsValidCargoType) == 1 && std::ranges::count(spec.accepts_cargo, cargo) == 1;
}

/** @copydoc IsLayerBaselineSteelDefinition */
bool IsLayerBaselineSteelDefinition(IndustryType type)
{
	if (type >= NUM_INDUSTRYTYPES) return false;
	const auto &spec = *GetIndustrySpec(type);
	if (!MatchBaselineInput(spec, CT_IRON_ORE)) return false;
	const CargoType steel = GetCargoTypeByLabel(CT_STEEL);
	// Reuse only the shipped exact one-to-one path. Other multipliers need a published rational-work/recipe contract, not a guessed fallback.
	if (!IsValidCargoType(steel) || std::ranges::count_if(spec.produced_cargo, IsValidCargoType) != 1 ||
			std::ranges::count(spec.produced_cargo, steel) != 1 || std::ranges::any_of(spec.production_rate, [](uint8_t rate) { return rate != 0; })) return false;
	const size_t input = static_cast<size_t>(std::ranges::find(spec.accepts_cargo, GetCargoTypeByLabel(CT_IRON_ORE)) - spec.accepts_cargo.begin());
	const size_t output = static_cast<size_t>(std::ranges::find(spec.produced_cargo, steel) - spec.produced_cargo.begin());
	return spec.input_cargo_multiplier[input][output] == 256;
}

/** @copydoc GetLayerBaselineReceiverType */
IndustryType GetLayerBaselineReceiverType(CargoLabel resource)
{
	// Resolve actual active meanings and supported production, retaining the first stable existing identity; never allocate a duplicate receiver.
	for (IndustryType type = 0; type < NUM_INDUSTRYTYPES; ++type) {
		if (resource == CT_IRON_ORE && IsLayerBaselineSteelDefinition(type)) return type;
		if (resource == CT_COAL) {
			const auto &spec = *GetIndustrySpec(type);
			// A compatible accept-only Coal identity remains read-only here; the absent utility producer supplies useful-power mutations later.
			if (MatchBaselineInput(spec, CT_COAL) && std::ranges::none_of(spec.produced_cargo, IsValidCargoType)) return type;
		}
	}
	return IT_INVALID;
}

/** @param industry Actual supported steelworks. @return Whether construction, pause, profile, calendar and physical layer permit conversion/intake. */
static bool IsBaselineSteelActive(const Industry &industry)
{
	return IsLayerBaselineSteelDefinition(industry.type) && _game_mode == GameMode::Normal && _pause_mode.None() &&
		industry.prod_level != PRODLEVEL_CLOSURE && GetTileLayer(industry.location.tile) == MapLayer::Surface &&
		IsLayerIndustryComplete(industry) && TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date).year >= TimerGameCalendar::Year{1950};
}

/** @copydoc GetLayerBaselineSteelInputRoom */
uint32_t GetLayerBaselineSteelInputRoom(const Industry &industry, CargoType cargo)
{
	// Only the validated role may use this owner; same-label cargo in an unavailable or different facility is not admission.
	if (cargo != GetCargoTypeByLabel(CT_IRON_ORE) || !IsBaselineSteelActive(industry)) return 0;
	const auto slot = std::ranges::find(industry.accepted, cargo, &Industry::AcceptedCargo::cargo);
	if (slot == industry.accepted.end()) return 0;
	const uint32_t accepted = slot->history == nullptr ? 0 : (*slot->history)[THIS_MONTH].accepted;
	return std::min<uint32_t>(UINT16_MAX - slot->waiting, UINT16_MAX - accepted);
}

/** @copydoc ReceiveLayerBaselineSteelCargo */
uint32_t ReceiveLayerBaselineSteelCargo(Industry &industry, CargoType cargo, uint32_t offered)
{
	const uint32_t amount = std::min(offered, GetLayerBaselineSteelInputRoom(industry, cargo));
	if (amount == 0) return 0;
	auto slot = std::ranges::find(industry.accepted, cargo, &Industry::AcceptedCargo::cargo);
	auto &history = slot->GetOrCreateHistory(); // Allocate before touching the physical stock.
	slot->waiting += static_cast<uint16_t>(amount);
	history[THIS_MONTH].accepted += static_cast<uint16_t>(amount);
	slot->last_accepted = TimerGameEconomy::date;
	return amount;
}

/** @copydoc ProcessLayerBaselineSteel */
uint32_t ProcessLayerBaselineSteel(Industry &industry)
{
	if (!IsBaselineSteelActive(industry)) return 0;
	auto input = industry.GetCargoAccepted(GetCargoTypeByLabel(CT_IRON_ORE));
	auto output = std::ranges::find(industry.produced, GetCargoTypeByLabel(CT_STEEL), &Industry::ProducedCargo::cargo);
	// Restoring/malformed actual slots hold their material; the definition's matching labels cannot manufacture a missing physical slot.
	if (input == industry.accepted.end() || output == industry.produced.end()) return 0;
	const uint16_t amount = static_cast<uint16_t>(std::min({uint32_t{input->waiting}, static_cast<uint32_t>(UINT16_MAX - output->waiting),
		static_cast<uint32_t>(UINT16_MAX - output->history[THIS_MONTH].production)}));
	input->waiting -= amount;
	output->waiting += amount;
	output->history[THIS_MONTH].production += amount;
	if (amount != 0) industry.last_prod_year = TimerGameEconomy::year;
	return amount;
}

/** @copydoc DispatchLayerBaselineSteel */
uint32_t DispatchLayerBaselineSteel(Industry &industry)
{
	if (!IsBaselineSteelActive(industry)) return 0;
	auto output = std::ranges::find(industry.produced, GetCargoTypeByLabel(CT_STEEL), &Industry::ProducedCargo::cargo);
	if (output == industry.produced.end()) return 0;
	const uint32_t offered = std::min({uint32_t{output->waiting}, uint32_t{255}, static_cast<uint32_t>(UINT16_MAX - output->history[THIS_MONTH].transported)});
	ExactStationSupplyTicket best;
	// The reused steelworks uses the same exact packet owner as mines; rating limits throughput while all unadmitted finite Steel remains stored.
	for (const Station *station : industry.stations_near) {
		auto candidate = PrepareExactStationSupply(station->index, output->cargo, offered, {industry.index, SourceType::Industry}, MapLayer::Surface, industry.exclusive_consumer);
		if (candidate.amount == 0) continue;
		candidate.amount = static_cast<uint16_t>(std::min<uint32_t>(candidate.amount, (offered * (candidate.rating + 1) + 255) / 256));
		// Stable station identity resolves equal service ratings, so client iteration order cannot decide cargo custody.
		if (best.amount == 0 || candidate.rating > best.rating || (candidate.rating == best.rating && candidate.station < best.station)) best = candidate;
	}
	const uint16_t supplied = static_cast<uint16_t>(CommitExactStationSupply(best));
	output->waiting -= supplied;
	output->history[THIS_MONTH].transported += supplied;
	return supplied;
}

/** @copydoc LayerBaselineSteelHasStoredCargo */
bool LayerBaselineSteelHasStoredCargo(const Industry &industry)
{
	return IsLayerBaselineSteelDefinition(industry.type) && (std::ranges::any_of(industry.accepted, [](const auto &slot) { return slot.waiting != 0; }) ||
		std::ranges::any_of(industry.produced, [](const auto &slot) { return slot.waiting != 0; }));
}
