/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_purchase.cpp Ordinary command consumers of the actual material-paid launch and orbital factory owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8B
 */
#include "stdafx.h"
#include "space_purchase.h"
#include "space_structure.h"
#include "space_structure_cmd.h"
#include "launch_center_cmd.h"
#include "launch_operations.h"
#include "space_vehicle.h"
#include "engine_base.h"
#include "company_base.h"
#include "company_func.h"
#include "station_base.h"
#include "station_map.h"
#include "map_func.h"
#include "multilayer_content.h"
#include "safeguards.h"

/** @copydoc IsSpacePurchaseEngine */
bool IsSpacePurchaseEngine(EngineID engine)
{
	const auto *definition = Engine::GetIfValid(engine);
	// An unrelated engine or a stale numeric mapping cannot borrow a canonical Space recipe or its facility permissions.
	return definition != nullptr && definition->type == VehicleType::Space && GetSpaceEngine(definition->VehInfo<SpaceVehicleInfo>().model) == engine;
}

/** @copydoc GetSpacePurchaseAssembly */
SpaceStructure *GetSpacePurchaseAssembly(TileIndex tile)
{
	// Surface stations, padding and a camera-local coordinate cannot identify a native orbital facility.
	if (!IsValidTile(tile) || GetTileLayer(tile) != MapLayer::Space) return nullptr;
	SpaceStructure *found = nullptr;
	const auto x = TileX(tile), y = TilePlaneY(tile);
	// Derive admission from existing disjoint physical footprints, never a private depot/ID table or the nearest visible module.
	for (auto *site : SpaceStructure::Iterate()) {
		if (site->state.kind != SpaceBuildKind::Assembly) continue;
		const auto *definition = GetSpaceConstructionDefinition(site->state.kind);
		if (definition == nullptr) continue;
		// Only the actual module footprint supplies assembly permission; its reserved output approach and adjacent inventories remain different owners.
		if (x < site->state.x || y < site->state.y || x >= uint{site->state.x} + definition->width || y >= uint{site->state.y} + definition->height) continue;
		if (found != nullptr) return nullptr; // Invalid overlapping ownership must refuse rather than choose one facility by pool order.
		found = site;
	}
	return found;
}

/** Resolve the actual Surface workshop inside its original native launch-station footprint.
 * @param tile Real clicked world tile; joined rail/road/hub membership supplies no assembly permission.
 * @return Original native center incarnation, or an empty reference without a genuine workshop tile.
 */
static LaunchCenterRef PurchaseLaunchWorkshop(TileIndex tile)
{
	// A real Surface station tile is required; another layer or a linked freight facility cannot lend this producer's physical workshop.
	if (!IsValidTile(tile) || GetTileLayer(tile) != MapLayer::Surface || !IsTileType(tile, TileType::Station)) return {};
	if (GetStationType(tile) != StationType::LaunchCenter) return {};
	const auto *center = Station::GetIfValid(GetStationIndex(tile));
	// The original native launch anchor is required, even when its Station also owns linked transport facilities.
	if (center == nullptr || !center->launch_center) return {};
	const auto zone = GetLaunchFacilityZone(center->launch_center->tile, center->launch_center->rotation, LaunchFacilityZone::Workshop);
	// Pad, recovery and storage zones retain their distinct owners and cannot masquerade as the physical workshop purchase location.
	if (!zone || !zone->Contains(tile)) return {};
	return GetLaunchCenterReference(*center);
}

/** @copydoc PurchaseSpaceVehicleAtFacility */
SpacePurchaseResult PurchaseSpaceVehicleAtFacility(DoCommandFlags flags, TileIndex tile, EngineID engine, CargoType cargo)
{
	SpacePurchaseResult result;
	// Existing canonical content and an actual company are prerequisites; replacement flags cannot invent a missing old-hull settlement context.
	if (!Company::IsValidID(_current_company) || !IsSpacePurchaseEngine(engine) || flags.Test(DoCommandFlag::AutoReplace) ||
		GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return result;
	const auto *definition = Engine::Get(engine);
	const auto model = definition->VehInfo<SpaceVehicleInfo>().model;
	const auto default_cargo = definition->GetDefaultCargoType();
	// Unsupported Space refits do not acquire permission through the ordinary command's temporary-purchase estimate path.
	if (!IsValidCargoType(default_cargo) || (cargo != INVALID_CARGO && cargo != default_cargo)) return result;
	// Query-cost remains a pure quote even if a caller also supplied Execute; no job, hull, definition or packet may appear during inspection.
	if (flags.Test(DoCommandFlag::QueryCost)) flags.Reset(DoCommandFlag::Execute);
	const auto center_ref = PurchaseLaunchWorkshop(tile);
	if (center_ref != LaunchCenterRef{}) {
		// The legacy Earth producer expects its command caller to check funds. Quote its real recipe first, then use the common company gate before any native hull allocation.
		result.cost = CmdAssembleLaunchCraft({}, center_ref.station, center_ref.generation, model);
		if (result.cost.Failed() || !CheckCompanyHasMoney(result.cost)) return result;
		if (flags.Test(DoCommandFlag::Execute)) result.cost = CmdAssembleLaunchCraft(flags, center_ref.station, center_ref.generation, model);
		// The Earth producer creates an actual paid Assembling hull, whose real workshop reference is the ordinary command result.
		if (result.cost.Succeeded() && flags.Test(DoCommandFlag::Execute)) {
			const auto *center = ResolveLaunchCenter(center_ref);
			assert(center != nullptr && center->launch_operations);
			result.vehicle = center->launch_operations->workshop.vehicle;
			assert(result.vehicle != VehicleID::Invalid());
		}
		return result;
	}
	auto *factory = GetSpacePurchaseAssembly(tile);
	if (factory == nullptr) return result;
	SpaceStructure *source = nullptr;
	// Ask the original synchronized producer to quote each actual inventory; its complete recipe/funds/source/berth gate remains the sole admission authority.
	for (auto *candidate : SpaceStructure::Iterate()) {
		auto quote = CmdQueueOrbitalManufacture({}, factory->index, factory->generation, candidate->index, candidate->generation, model);
		if (quote.Failed()) continue;
		// The generic command has no explicit inventory argument. Multiple eligible physical sources require the existing explicit source-selection command, never pool-order guessing.
		if (source != nullptr) return {};
		source = candidate; result.cost = std::move(quote);
	}
	if (source == nullptr) return result;
	result.factory = GetSpaceStructureReference(*factory);
	if (flags.Test(DoCommandFlag::Execute)) {
		result.cost = CmdQueueOrbitalManufacture(flags, factory->index, factory->generation, source->index, source->generation, model);
		if (result.cost.Succeeded()) result.serial = factory->module->manufacture->serial;
	}
	return result;
}
