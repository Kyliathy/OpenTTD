/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file multilayer_space_content.cpp Steel/Goods Space content and stable compatibility signature. @see docs/zz-rhr/upgrades/2026-10/r2mlvs1-e-ground-and-space-budgets.md. */
#include "stdafx.h"
#include "multilayer_space_content.h"
#include "cargotype.h"
#include "multilayer_bootstrap.h"
#include "space_mining_tuning.h"
#include <array>
#include <limits>
#include "safeguards.h"

static constexpr MultiLayerRecipeCargo rocket[] = {{CargoLabel{"STEL"}, 12}, {CargoLabel{"GOOD"}, 16}};
static constexpr MultiLayerRecipeCargo probe[] = {{CargoLabel{"STEL"}, 4}, {CargoLabel{"GOOD"}, 8}};
static constexpr MultiLayerRecipeCargo survey[] = {{CargoLabel{"STEL"}, 2}, {CargoLabel{"GOOD"}, 4}};
static constexpr MultiLayerRecipeCargo pod[] = {{CargoLabel{"STEL"}, 1}, {CargoLabel{"GOOD"}, 2}};
static constexpr MultiLayerRecipeCargo mirror[] = {{CargoLabel{"STEL"}, 4}, {CargoLabel{"GOOD"}, 4}};
static constexpr MultiLayerRecipeCargo core[] = {{CargoLabel{"STEL"}, 16}, {CargoLabel{"GOOD"}, 24}};
static constexpr MultiLayerRecipeCargo dock[] = {{CargoLabel{"STEL"}, 8}, {CargoLabel{"GOOD"}, 12}};
static constexpr MultiLayerRecipeCargo storage[] = {{CargoLabel{"STEL"}, 8}, {CargoLabel{"GOOD"}, 8}};
static constexpr MultiLayerRecipeCargo assembly[] = {{CargoLabel{"STEL"}, 8}, {CargoLabel{"GOOD"}, 16}};
static constexpr MultiLayerRecipeCargo processor[] = {{CargoLabel{"STEL"}, 10}, {CargoLabel{"GOOD"}, 16}};
static constexpr MultiLayerRecipeCargo power[] = {{CargoLabel{"STEL"}, 6}, {CargoLabel{"GOOD"}, 20}};
static constexpr MultiLayerRecipeCargo fuel[] = {{CargoLabel{"STEL"}, 6}, {CargoLabel{"GOOD"}, 12}};
static constexpr MultiLayerRecipeCargo heavy[] = {{CargoLabel{"STEL"}, 6}, {CargoLabel{"GOOD"}, 12}, {CargoLabel{"RBPT"}, 4}, {CargoLabel{"ELEC"}, 2}};
static constexpr MultiLayerRecipeCargo service[] = {{CargoLabel{"RFMT"}, 1}, {CargoLabel{"ELEC"}, 1}};

/** @copydoc GetSpaceProbeServiceInputs */
std::span<const MultiLayerRecipeCargo> GetSpaceProbeServiceInputs() { return service; }
/** @copydoc CanCommissionSpaceVehicle */
bool CanCommissionSpaceVehicle(uint16_t model, MultiLayerCommissioningFacility facility, bool completed)
{
	if (!completed) return false;
	if (model == 14) return facility == MultiLayerCommissioningFacility::LaunchCenter;
	// Initial15/19/20 retain their paid Earth bootstrap; the appended heavy miner cannot acquire an unimplemented Surface assembly/payload route.
	if (model == 15 || model == 19 || model == 20) return facility == MultiLayerCommissioningFacility::RocketPayload || facility == MultiLayerCommissioningFacility::OrbitalAssembly;
	// Advanced roles require an actual completed orbital Assembly, independently of engine availability or a price quote.
	return (model == 16 || model == 17 || model == SPACE_HEAVY_MINER_MODEL) && facility == MultiLayerCommissioningFacility::OrbitalAssembly;
}

static const SpaceConstructionDefinition definitions[] = {
	{SpaceBuildKind::Rocket, 14, rocket, 400000, 148, 320, TimerGameCalendar::Year{1990}},
	{SpaceBuildKind::MiningProbe, 15, probe, 100000, 148, 128, TimerGameCalendar::Year{1990}},
	{SpaceBuildKind::SurveyProbe, 19, survey, 60000, 148, 64, TimerGameCalendar::Year{1990}},
	{SpaceBuildKind::ConstructionProbe, 20, probe, 100000, 148, 128, TimerGameCalendar::Year{1990}},
	{SpaceBuildKind::Pod, 18, pod, 2000, 74, 32, TimerGameCalendar::Year{1990}},
	{SpaceBuildKind::Mirror, UINT16_MAX, mirror, 40000, 148, 96, TimerGameCalendar::Year{1990}},
	{SpaceBuildKind::Core, UINT16_MAX, core, 700000, 740, 448, TimerGameCalendar::Year{1995}, 3, 3},
	{SpaceBuildKind::Dock, UINT16_MAX, dock, 160000, 444, 224, TimerGameCalendar::Year{1995}, 2, 2},
	{SpaceBuildKind::Storage, UINT16_MAX, storage, 120000, 296, 192, TimerGameCalendar::Year{1995}, 2, 2},
	{SpaceBuildKind::Assembly, UINT16_MAX, assembly, 180000, 592, 256, TimerGameCalendar::Year{1995}, 2, 2},
	{SpaceBuildKind::Steelworks, UINT16_MAX, processor, 220000, 740, 288, TimerGameCalendar::Year{1995}, 3, 3},
	{SpaceBuildKind::Goodsworks, UINT16_MAX, processor, 220000, 740, 288, TimerGameCalendar::Year{1995}, 3, 3},
	{SpaceBuildKind::Power, UINT16_MAX, power, 140000, 444, 256, TimerGameCalendar::Year{1995}, 2, 2},
	{SpaceBuildKind::FuelStore, UINT16_MAX, fuel, 100000, 296, 192, TimerGameCalendar::Year{1995}, 2, 2},
	{SpaceBuildKind::HeavyRobot, 16, heavy, 200000, 592, 288, TimerGameCalendar::Year{1995}},
	{SpaceBuildKind::Hauler, 17, heavy, 200000, 592, 288, TimerGameCalendar::Year{1995}},
	{SpaceBuildKind::Refinery, UINT16_MAX, processor, 220000, 740, 288, TimerGameCalendar::Year{1995}, 3, 3},
	{SpaceBuildKind::IceProcessor, UINT16_MAX, processor, 220000, 740, 288, TimerGameCalendar::Year{1995}, 3, 3},
	{SpaceBuildKind::HeavyMiner, SPACE_HEAVY_MINER_MODEL, heavy, 200000, 592, 288, TimerGameCalendar::Year{1995}},
};

/** @copydoc GetSpaceConstructionDefinitions */
std::span<const SpaceConstructionDefinition> GetSpaceConstructionDefinitions() { return definitions; }
/** @copydoc GetSpaceConstructionDefinition */
const SpaceConstructionDefinition *GetSpaceConstructionDefinition(SpaceBuildKind kind)
{
	const auto found = std::ranges::find(definitions, kind, &SpaceConstructionDefinition::kind);
	return found == std::end(definitions) ? nullptr : &*found;
}
/** @copydoc GetSpaceVehicleConstruction */
const SpaceConstructionDefinition *GetSpaceVehicleConstruction(uint16_t model)
{
	if (model == UINT16_MAX) return nullptr;
	const auto found = std::ranges::find(definitions, model, &SpaceConstructionDefinition::model);
	return found == std::end(definitions) ? nullptr : &*found;
}
/** @copydoc GetMultiLayerSpaceBudget */
const MultiLayerSpaceBudget &GetMultiLayerSpaceBudget() { static constexpr MultiLayerSpaceBudget budget; return budget; }

/** @copydoc GetMultiLayerPayloadMass */
std::optional<uint64_t> GetMultiLayerPayloadMass(std::span<const MultiLayerRecipeCargo> cargo)
{
	uint64_t total = 0;
	// Accumulate native mass before rounding so many half-tonne Goods crates cannot bypass the complete payload bound.
	for (const auto &item : cargo) {
		const auto type = GetCargoTypeByLabel(item.label);
		// Missing/invalid physical cargo meanings cannot turn a declared material into a zero-mass payload.
		if (type >= NUM_CARGO || !CargoSpec::Get(type)->IsValid()) return std::nullopt;
		const uint64_t mass = uint64_t{item.quantity} * CargoSpec::Get(type)->weight;
		if (total > std::numeric_limits<uint64_t>::max() - mass) return std::nullopt;
		total += mass;
	}
	return total;
}

/** @copydoc GetMultiLayerSpaceContentSignatureForVersion */
uint64_t GetMultiLayerSpaceContentSignatureForVersion(uint16_t version)
{
	if (!IsSupportedSpaceConstructionVersion(version)) return 0;
	const bool legacy = version == MULTILAYER_SPACE_LEGACY_CONTENT_VERSION;
	uint64_t digest = 14695981039346656037ULL;
	/** @param value Canonical unsigned scalar, folded in the original fixed eight-byte order on every platform. */
	auto hash = [&](uint64_t value) { for (uint shift = 0; shift < 64; shift += 8) { digest ^= (value >> shift) & 0xFF; digest *= 1099511628211ULL; } };
	/** @param value Fixed native label; retain the original scalar-per-character folding so older paid content fingerprints remain exact. */
	auto label_hash = [&](const BaseLabel &value) { for (uint8_t byte : value) hash(byte); };
	hash(version);
	// Legacy2 fingerprints include precisely their original ordered recipes. Appended heavy content must not invalidate or rewrite already paid hull/module provenance.
	for (const auto &definition : definitions) {
		if (legacy && definition.kind >= SpaceBuildKind::HeavyMiner) continue;
		hash(to_underlying(definition.kind)); hash(definition.model); hash(definition.capital); hash(definition.work_ticks);
		hash(definition.dry_mass_sixteenths); hash(definition.introduction_year.base()); hash(definition.width); hash(definition.height);
		for (const auto &input : definition.inputs) { label_hash(input.label); hash(input.quantity); }
		hash(UINT64_MAX);
	}
	const auto &b = GetMultiLayerSpaceBudget();
	// Shared legacy budgets retain their original field order; per-miner additions are a separate current-revision suffix below.
	for (uint64_t value : std::array<uint64_t, 31>{b.rocket_payload_sixteenths, b.probe_hold_sixteenths, b.fuel_hold, b.outbound_fuel, b.recovery_fuel,
		b.prepare_ticks, b.turnaround_ticks, b.flight_ticks, b.descent_ticks, b.service_ticks, b.scan_ticks, b.active_per_day,
		b.idle_per_day, b.pad_per_day, b.mission_fee, b.returns_per_service, b.starting_pods, b.mining_interval, b.mining_units, b.stock_cap,
		b.launch_propellant_cap, b.launch_metal_cap, b.launch_electronics_cap, b.launch_components_cap, b.launch_pod_cap, b.probe_metal_cap,
		b.probe_electronics_cap, b.core_activation_ticks, b.core_panel_w, b.module_panel_w, b.work_j_per_tick}) hash(value);
	for (const auto &input : service) { label_hash(input.label); hash(input.quantity); }
	// Bind the same shared Space recipes, physical stock/residue caps and utility requirements as their actual all-input production owner.
	for (const auto &manufacturing : GetMultiLayerManufacturingRecipes()) {
		if (manufacturing.layer != MapLayer::Space) continue;
		hash(manufacturing.introduction_year.base()); hash(manufacturing.input_capacity); hash(manufacturing.output_capacity);
		hash(manufacturing.residue_mass_sixteenths);
		const auto &r = manufacturing.recipe;
		label_hash(r.id); hash(r.work_economy_ticks); hash(r.max_batches); label_hash(r.reserve_label); hash(r.reserve_per_batch);
		hash(r.heat_j_per_batch); hash(r.power_j_per_batch); hash(r.aggregate_per_batch);
		hash(r.inputs.size()); hash(r.outputs.size());
		for (const auto &input : manufacturing.recipe.inputs) { label_hash(input.label); hash(input.quantity); }
		for (const auto &output : manufacturing.recipe.outputs) { label_hash(output.label); hash(output.quantity); }
	}
	// Old fingerprints stop at their original0..20 model set; current content additionally binds explicit physical pursuit definitions.
	for (const auto &vehicle : GetMultiLayerVehicleCapabilities()) {
		if (vehicle.model_id < 14 || (legacy && vehicle.model_id >= SPACE_HEAVY_MINER_MODEL)) continue;
		hash(vehicle.model_id); hash(to_underlying(vehicle.transport)); hash(to_underlying(vehicle.commissioning)); hash(to_underlying(vehicle.service));
		hash(vehicle.cargo_classes.base()); hash(vehicle.supported_layers.base());
		hash(vehicle.introduction_year.base()); hash(vehicle.cargo_labels.size());
		for (auto label : vehicle.cargo_labels) label_hash(label);
		if (!legacy) {
			hash(vehicle.mining_tuning != nullptr);
			if (vehicle.mining_tuning != nullptr) {
				const auto &motion = *vehicle.mining_tuning;
				hash(motion.travel_tiles_per_day); hash(motion.approach_tolerance_q16); hash(motion.minimum_approach_ticks);
				hash(motion.closing_tiles_per_day); hash(motion.approach_margin_ticks);
			}
		}
	}
	// Surface supply prices/caps remain part of the original recipe economy; appending a miner cannot silently alter those existing meanings.
	for (const auto &supply : GetMultiLayerSurfaceSupplies()) {
		label_hash(supply.label); hash(supply.introduction_year.base()); hash(supply.price_per_unit); hash(supply.capacity_per_day);
	}
	if (!legacy) {
		hash(2); // Exact number of explicit mining operating definitions, with their stable role identities included below.
		// Bind both the unchanged small miner and the appended heavy operating ledger; native consumers cannot use changed service/extraction rates under the same content identity.
		for (uint16_t model : {uint16_t{15}, SPACE_HEAVY_MINER_MODEL}) {
			const auto *miner = GetSpaceMiningTuning(model); assert(miner != nullptr);
			for (uint64_t value : std::array<uint64_t, 11>{miner->model, miner->hold_sixteenths, miner->extraction_units, miner->extraction_ticks,
				miner->active_per_day, miner->idle_per_day, miner->service_ticks, miner->service_metal, miner->service_electronics,
				miner->returns_per_service, miner->starting_pods}) hash(value);
		}
	}
	return digest;
}

/** @copydoc GetMultiLayerSpaceContentSignature */
uint64_t GetMultiLayerSpaceContentSignature() { return GetMultiLayerSpaceContentSignatureForVersion(MULTILAYER_SPACE_CONTENT_VERSION); }

/** @copydoc IsSupportedSpaceConstructionVersion */
bool IsSupportedSpaceConstructionVersion(uint16_t version)
{
	return version == MULTILAYER_SPACE_LEGACY_CONTENT_VERSION || version == MULTILAYER_SPACE_CONTENT_VERSION;
}

/** @copydoc IsSupportedSpaceVehicleContentVersion */
bool IsSupportedSpaceVehicleContentVersion(uint16_t version, uint16_t model)
{
	// Existing paid roles keep their exact2 receipts, while the appended21 role must prove its own3 provenance and real construction definition.
	return IsSupportedSpaceConstructionVersion(version) && GetSpaceVehicleConstruction(model) != nullptr &&
		(version != MULTILAYER_SPACE_LEGACY_CONTENT_VERSION || model < SPACE_HEAVY_MINER_MODEL);
}

/** @copydoc IsSupportedMultiLayerSpaceContent */
bool IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID profile, uint16_t version, uint64_t signature)
{
	// Classic has no Space-content authority; a matching nonzero Space fingerprint cannot opt it into another gameplay profile.
	if (profile == MultiLayerGameplayProfileID::Classic) return version == 0 && signature == 0;
	// Resolve the exact supported revision rather than comparing old paid assets with newly appended definitions; file readers still enforce source-version admission independently.
	return profile == MultiLayerGameplayProfileID::MultilayerIndustry && IsSupportedSpaceConstructionVersion(version) && signature == GetMultiLayerSpaceContentSignatureForVersion(version);
}
