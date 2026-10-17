/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_purchase.cpp Ordinary purchase contracts using actual native factory, packet, Engine and common-grant owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8B
 * Frames/packets/content are explicit unit inputs; separate native observers prove command payment and gameplay assembly.
 */
#include "../stdafx.h"
#include "mock_environment.h"
#include "../space_purchase.h"
#include "../vehicle_cmd.h"
#include "../engine_base.h"
#include "../space_manifest.h"
#include "manufacturing_fixture.h"
#include "../safeguards.h"

/** @param rig Original isolated native factory world. @return Its real world-layer footprint anchor, never a camera-local coordinate. */
static TileIndex PurchaseTile(const ManufacturingReservationRig &rig)
{
	return TileXYL(rig.assembly->state.x, rig.assembly->state.y, MapLayer::Space);
}

/** Each existing canonical role uses the ordinary command to create only one paid request, then its original public work/materialization owner creates the actual product. */
TEST_CASE("Ordinary Space purchase queues each canonical factory recipe", "[manufacturing-purchase]")
{
	// Fresh isolated worlds separate every role's packets and work. Explicit unit Engine definitions characterize existing catalogue lookup, not gameplay catalogue commissioning.
	for (uint16_t model : {15, 19, 20, 16, 17}) {
		CAPTURE(model); ManufacturingMaterializationRig rig; rig.Supply(model); REQUIRE(EnsureSpaceEngineDefinition(model));
		const auto engine = GetSpaceEngine(model); const auto &recipe = *GetSpaceVehicleConstruction(model);
		const auto before = GetNextSpaceVehicleGeneration();
		auto [cost, vehicle, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute, PurchaseTile(rig), engine, true, INVALID_CARGO, ClientID::Invalid);
		REQUIRE(cost.Succeeded()); CHECK(cost.GetCost() == Money{recipe.capital}); CHECK(vehicle == VehicleID::Invalid()); CHECK(mail == 0);
		CHECK(capacity == GetSpaceFreightCapacity(model, Engine::Get(engine)->GetDefaultCargoType()));
		CHECK(capacities[Engine::Get(engine)->GetDefaultCargoType()] == capacity); CHECK(GetSpaceVehicleCount() == 0); CHECK(GetNextSpaceVehicleGeneration() == before);
		REQUIRE(rig.assembly->module->manufacture != nullptr); auto &job = *rig.assembly->module->manufacture;
		CHECK(job.phase == OrbitalManufacturePhase::Queued); CHECK(job.model == model); CHECK(job.source == GetSpaceStructureReference(*rig.core));
		CHECK(job.capital_paid == recipe.capital); CHECK(job.work_ticks == 0); CHECK(job.product == SpaceCraftRef{});
		rig.Tick(); CHECK(job.phase == OrbitalManufacturePhase::Reserved); CHECK(job.work_ticks == 0);
		rig.Work(recipe.work_ticks);
		REQUIRE(job.phase == OrbitalManufacturePhase::Complete); REQUIRE(ResolveSpaceCraft(job.product) != nullptr);
		CHECK(GetSpaceVehicleCount() == 1); CHECK(GetSpaceVehicleModel(*ResolveSpaceCraft(job.product)) == model);
		CHECK(ResolveSpaceCraft(job.product)->construction.assembly_serial == job.serial);
		// Completion must leave every native packet container valid and empty, including the heavy recipe's final components, before world cleanup can retire their owners.
		for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) {
			CAPTURE(input);
			CHECK(job.inputs[input].TotalCount() == 0);
			CHECK(job.inputs[input].Packets()->begin() == job.inputs[input].Packets()->end());
		}
	}
}

/** Both ordinary and query-cost inspection retain every actual packet/epoch/identity; even mixed Execute/QueryCost flags cannot reserve a hidden batch. */
TEST_CASE("Ordinary Space purchase quotes never create jobs or temporary hulls", "[manufacturing-purchase]")
{
	ManufacturingMaterializationRig rig; rig.Supply(16); REQUIRE(EnsureSpaceEngineDefinition(16));
	const auto structures = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(), generation = GetNextSpaceVehicleGeneration();
	const auto money = Company::Get(_current_company)->money; const auto random = _random;
	// These are public command estimates with the same existing Engine/facility; no temporary refit purchase or output identity is needed to calculate their capacity/capital.
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::QueryCost}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) {
		auto [cost, vehicle, capacity, mail, capacities] = CmdBuildVehicle(flags, PurchaseTile(rig), GetSpaceEngine(16), false, INVALID_CARGO, ClientID::Invalid);
		CHECK(cost.Succeeded()); CHECK(cost.GetCost() == Money{GetSpaceVehicleConstruction(16)->capital}); CHECK(vehicle == VehicleID::Invalid()); CHECK(capacity == 0); CHECK(mail == 0);
		CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(GetNextSpaceVehicleGeneration() == generation);
		CHECK(Company::Get(_current_company)->money == money); CHECK(GetSpaceVehicleCount() == 0); CHECK(rig.assembly->module->manufacture == nullptr);
		CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	}
}

/** An ordinary command without a source parameter must not guess between two real complete inventories; the original explicit source-selection command remains available. */
TEST_CASE("Ordinary Space purchase requires an unambiguous physical inventory", "[manufacturing-purchase]")
{
	ManufacturingMaterializationRig rig; rig.Supply(19); REQUIRE(EnsureSpaceEngineDefinition(19));
	// Both original inventories contain an actual complete bill; neither shares packets or gains automatic priority through its pool ID.
	for (const auto &input : GetSpaceVehicleConstruction(19)->inputs) rig.Add(*rig.storage, input.label, static_cast<uint16_t>(input.quantity));
	const auto before = ComputeSpaceStructureDigest();
	auto [cost, vehicle, capacity, mail, capacities] = CmdBuildVehicle(DoCommandFlag::Execute, PurchaseTile(rig), GetSpaceEngine(19), false, INVALID_CARGO, ClientID::Invalid);
	CHECK(cost.Failed()); CHECK(vehicle == VehicleID::Invalid()); CHECK(GetSpaceVehicleCount() == 0); CHECK(ComputeSpaceStructureDigest() == before);
	CHECK(CmdQueueOrbitalManufacture({}, rig.assembly->index, rig.assembly->generation, rig.storage->index, rig.storage->generation, 19).Succeeded());
	CHECK(ComputeSpaceStructureDigest() == before);
}

/** Wrong physical layer, lost rights/funds/power/material, unsupported refit and replacement flags all refuse before the common paid owner changes. */
TEST_CASE("Ordinary Space purchase cannot bypass physical rights or recipe gates", "[manufacturing-purchase]")
{
	ManufacturingMaterializationRig rig; rig.Supply(16); REQUIRE(EnsureSpaceEngineDefinition(16));
	TileIndex tile = PurchaseTile(rig); CargoType cargo = INVALID_CARGO; DoCommandFlags flags = DoCommandFlag::Execute;
	SECTION("Surface coordinates do not identify the same orbital factory") { tile = TileXYL(rig.assembly->state.x, rig.assembly->state.y, MapLayer::Surface); }
	SECTION("The output approach is not the factory footprint") { const auto &recipe = *GetSpaceConstructionDefinition(SpaceBuildKind::Assembly); tile = TileXYL(rig.assembly->state.x, rig.assembly->state.y + recipe.height, MapLayer::Space); }
	SECTION("Invalid world geometry grants nothing") { tile = INVALID_TILE; }
	SECTION("Refit estimates cannot create a temporary paid hull") { cargo = GetCargoTypeByLabel(CargoLabel{"GOOD"}); flags.Reset(DoCommandFlag::Execute); }
	SECTION("Replacement requires its original-hull settlement contract") { flags.Set(DoCommandFlag::AutoReplace); }
	SECTION("Unowned facility grants no operating right") { rig.assembly->state.owner = OWNER_NONE; }
	SECTION("Unowned source grants no physical materials") { rig.core->state.owner = OWNER_NONE; }
	SECTION("Insufficient capital cannot reserve a recipe") { Company::Get(_current_company)->money = 0; }
	SECTION("A paused original panel supplies no ready facility") { REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, rig.core->index, rig.core->generation, true).Succeeded()); }
	SECTION("A missing last component cannot reserve earlier labels") { REQUIRE(HubCargoTransfer::Consume(rig.core->module->stock[GetCargoTypeByLabel(CargoLabel{"ELEC"})], 1)); }
	const auto before = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(), generation = GetNextSpaceVehicleGeneration();
	const auto random = _random; const auto money = Company::Get(_current_company)->money;
	auto [cost, vehicle, capacity, mail, capacities] = CmdBuildVehicle(flags, tile, GetSpaceEngine(16), true, cargo, ClientID::Invalid);
	CHECK(cost.Failed()); CHECK(vehicle == VehicleID::Invalid()); CHECK(GetSpaceVehicleCount() == 0); CHECK(GetNextSpaceVehicleGeneration() == generation);
	CHECK(ComputeSpaceStructureDigest() == before); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Company::Get(_current_company)->money == money);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}
