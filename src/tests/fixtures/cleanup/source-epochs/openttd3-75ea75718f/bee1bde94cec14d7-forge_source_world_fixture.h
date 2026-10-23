/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file forge_source_world_fixture.h Borrowed original finite CORE/Coal sources and real native RFMT refinery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15FBB/15FBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Native pools own all quantities; this view retains identities for later original-world transport and files.
 */
#ifndef TESTS_FORGE_SOURCE_WORLD_FIXTURE_H
#define TESTS_FORGE_SOURCE_WORLD_FIXTURE_H
#include "shield_source_world_fixture.h"

/** Borrowed original source participants, preserving both actual finite claims and every paid raw carrier. */
struct ForgeSourceWorld {
	std::array<Train *, 4> parts{}; ///< Actual CORE front/wagon, then actual Coal front/wagon; native coupling remains authoritative.
	std::array<Industry *, 3> industries{}; ///< Actual CORE mine, Coal mine and Surface CFMT, without substituted inventories.
	std::array<GeologyDepositID, 2> deposits{}; ///< Original generated CORE/Coal identities from the accepted empty world.
	std::array<TileIndex, 2> homes{}; ///< Actual paid Underground raw depots, never an arrived pose.
	std::array<CargoType, 3> cargos{}; ///< Native CORE, Coal and RFMT meanings rebuilt by ordinary application loading.
};

/** @param source_only Require the original four-part/three-industry/station-free/packet-free stopped empty raw world; later explicit route consumers validate their expanded pools separately. @return Borrowed exact original ordinary-loaded participants, with no source construction, registry, date, funding or quantity repair. */
inline ForgeSourceWorld LoadForgeSourceWorld(bool source_only = true)
{
	ForgeSourceWorld world;
	const auto parts = ShieldSourceValues("OTTD_FORGE_SOURCE_PARTS", 4), references = ShieldSourceValues("OTTD_FORGE_SOURCE_REFERENCES", 7);
	if (source_only) {
		REQUIRE(Vehicle::GetNumItems() == 4); REQUIRE(Industry::GetNumItems() == 3); REQUIRE(Station::GetNumItems() == 0); REQUIRE(CargoPacket::GetNumItems() == 0);
	}
	// Exact original native pool identities must survive. Cargo totals alone
	// cannot substitute a new source, refinery, front or coupled raw wagon.
	for (size_t index = 0; index < world.parts.size(); ++index) {
		REQUIRE(parts[index] < UINT32_MAX); const auto id = VehicleID{static_cast<uint32_t>(parts[index])};
		const auto *vehicle = Vehicle::GetIfValid(id); REQUIRE(vehicle != nullptr); REQUIRE(vehicle->type == VehicleType::Train);
		world.parts[index] = Train::Get(id); REQUIRE(world.parts[index]->owner == _current_company);
	}
	for (size_t index = 0; index < world.industries.size(); ++index) {
		REQUIRE(references[index] < UINT16_MAX); world.industries[index] = Industry::GetIfValid(IndustryID{static_cast<uint16_t>(references[index])}); REQUIRE(world.industries[index] != nullptr);
	}
	world.cargos = {GetCargoTypeByLabel(CargoLabel{"CORE"}), GetCargoTypeByLabel(CargoLabel{"COAL"}), GetCargoTypeByLabel(CargoLabel{"RFMT"})};
	for (const auto cargo : world.cargos) REQUIRE(cargo != INVALID_CARGO);
	// Both original finite claims and homes remain genuine native owners.
	// Source-only files additionally retain whole stopped/empty storage; later
	// physical routes may borrow the same originals without hiding their travel.
	for (size_t index = 0; index < world.deposits.size(); ++index) {
		REQUIRE(references[index + 3] < UINT32_MAX); world.deposits[index] = GeologyDepositID{static_cast<uint32_t>(references[index + 3])};
		REQUIRE(references[index + 5] < Map::Size()); world.homes[index] = TileIndex{static_cast<uint32_t>(references[index + 5])};
		const auto *deposit = GetGeologyWorldState().deposits.Get(world.deposits[index]); REQUIRE(deposit != nullptr);
		const auto label = CargoSpec::Get(world.cargos[index])->label; REQUIRE(deposit->resource == label); REQUIRE(deposit->layer == MapLayer::Underground);
		REQUIRE(world.industries[index]->type == GetLayerMineIndustryType(label)); REQUIRE(world.industries[index]->produced[0].cargo == world.cargos[index]);
		const auto *mine = GetLayerMineState(world.industries[index]->index); REQUIRE(mine != nullptr); REQUIRE(mine->deposit == world.deposits[index]);
		auto *front = world.parts[index * 2], *wagon = world.parts[index * 2 + 1];
		REQUIRE(front->engine_type == GetUndergroundEngine(4)); REQUIRE(wagon->engine_type == GetUndergroundEngine(5)); REQUIRE(wagon->cargo_type == world.cargos[index]);
		REQUIRE(front->Next() == wagon); REQUIRE(wagon->First() == front); REQUIRE(wagon->Next() == nullptr); REQUIRE(IsTrainOnItsTiles(front)); REQUIRE_FALSE(front->vehstatus.Test(VehState::Crashed));
		REQUIRE(GetTileLayer(world.homes[index]) == MapLayer::Underground);
		if (source_only) {
			REQUIRE(front->IsChainInDepot()); REQUIRE(front->tile == world.homes[index]); REQUIRE(front->vehstatus.Test(VehState::Stopped));
			REQUIRE(front->cargo.StoredCount() == 0); REQUIRE(wagon->cargo.StoredCount() == 0);
		}
	}
	const auto *recipe = GetLayerProcessorRecipe(*world.industries[2]); REQUIRE(recipe != nullptr); REQUIRE(recipe->recipe.id == MultiLayerRecipeID{"CFMT"});
	return world;
}

#endif /* TESTS_FORGE_SOURCE_WORLD_FIXTURE_H */
