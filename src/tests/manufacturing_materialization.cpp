/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_materialization.cpp Isolated paid-work, final material and real native capacity/provenance contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8
 * Explicit unit frames/packets/package descriptors characterize owner boundaries; separate native observers prove gameplay commissioning.
 */
#include "../stdafx.h"
#include "mock_environment.h"
#include "manufacturing_fixture.h"
#include "../space_manifest.h"
#include "../engine_base.h"
#include "../vehicle_func.h"
#include "../group.h"
#include "../newgrf_config.h"
#include "../safeguards.h"

/** Every supported role embodies exactly one full original recipe after its own fully paid work, with real native unit/berth and original factory provenance. */
TEST_CASE("Orbital manufacturing materializes each canonical role exactly once", "[manufacturing-materialization]")
{
	// Each role owns a fresh contract world, so another model's engine, inputs or partially paid work cannot commission it for free.
	for (uint16_t model : {15, 19, 20, 16, 17}) {
		CAPTURE(model); ManufacturingMaterializationRig rig; auto &job = rig.Start(model); const auto &recipe = *GetSpaceVehicleConstruction(model);
		const auto spent = GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j;
		rig.Work(recipe.work_ticks - 1); REQUIRE(job.phase == OrbitalManufacturePhase::Working); CHECK(job.work_ticks == recipe.work_ticks - 1);
		CHECK(GetSpaceVehicleCount() == 0); CHECK(job.product == SpaceCraftRef{});
		// All input packets remain original reserved stock through the final pre-completion checkpoint; no partial bill is lost on a blocked materialization.
		for (size_t input = 0; input < recipe.inputs.size(); ++input) CHECK(job.inputs[input].TotalCount() == recipe.inputs[input].quantity);
		rig.Tick(); REQUIRE(job.phase == OrbitalManufacturePhase::Complete); REQUIRE(GetSpaceVehicleCount() == 1);
		auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr); CHECK(GetSpaceVehicleModel(*product) == model);
		CHECK(product->construction.assembly == GetSpaceStructureReference(*rig.assembly)); CHECK(product->construction.assembly_serial == job.serial);
		CHECK(product->construction.home == LaunchCenterRef{}); CHECK(product->construction.origin == LaunchCenterRef{}); CHECK(product->construction.surface_tile == INVALID_TILE);
		CHECK(GetSpaceStructureBerth(*product, *rig.assembly) == std::optional<uint8_t>{0}); CHECK(ValidateSpaceProduction());
		CHECK(job.work_energy_j == uint64_t{recipe.work_ticks} * 750); CHECK(GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j == spent + job.work_energy_j);
		// The one actual hull receives the complete consumed bill only at this commit, and every unused recipe slot remains canonically empty.
		for (size_t input = 0; input < MAX_ORBITAL_MANUFACTURE_INPUTS; ++input) {
			CHECK(job.inputs[input].TotalCount() == 0); CHECK(job.consumed[input] == (input < recipe.inputs.size() ? recipe.inputs[input].quantity : 0));
		}
		const auto original = job.product; const auto digest = ComputeSpaceStructureDigest();
		{ AutoRestoreBackup pause(_pause_mode, PauseModes{}); TickOrbitalManufacturingWork(); TickOrbitalManufacturingWork(); }
		CHECK(job.product == original); CHECK(GetSpaceVehicleCount() == 1); CHECK(ComputeSpaceStructureDigest() == digest);
		CHECK(ValidateOrbitalManufacturingProductionLinks(CaptureSpaceStructures(), CaptureSpaceProduction(), GetNextSpaceStructureGeneration(), GetNextSpaceVehicleGeneration()));
		REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, rig.assembly->index, rig.assembly->generation, rig.core->index, rig.core->generation).Succeeded());
		CHECK(rig.assembly->module->manufacture == nullptr); CHECK(ResolveSpaceCraft(original) == product); // Clearing a completed receipt cannot sell/delete or recreate its actual paid product.
	}
}

/** Actual source outage, pause and a repeated opportunity preserve the same partial work and complete four-label batch. */
TEST_CASE("Orbital manufacture work requires one actual full original grant per opportunity", "[manufacturing-materialization]")
{
	ManufacturingMaterializationRig rig; auto &job = rig.Start(16); rig.Work(7); const auto work = job.work_ticks; const auto energy = job.work_energy_j;
	const auto held = CaptureSpaceStructures(); const auto digest = ComputeSpaceStructureDigest();
	TickOrbitalManufacturingWork(); CHECK(ComputeSpaceStructureDigest() == digest); // Paused invocation cannot earn work.
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); TickOrbitalManufacturingWork(); CHECK(job.work_ticks == work); // Same actual already spent interval cannot run again.
	}
	REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, rig.core->index, rig.core->generation, true).Succeeded()); rig.Work(3);
	CHECK(job.work_ticks == work); CHECK(job.work_energy_j == energy); CHECK(CaptureSpaceStructures()[1].module->manufacture->inputs == held[1].module->manufacture->inputs);
	REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, rig.core->index, rig.core->generation, false).Succeeded()); rig.Tick(); CHECK(job.work_ticks == work + 1);
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); rig.Allocate();
		const auto release = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {rig.assembly->module->consumer, GetUtilityPowerInterval()}, 1);
		REQUIRE(CommitUtilityMutation(release).error == UtilityPowerError::None); const auto previous = job.work_ticks;
		TickOrbitalManufacturingWork(); CHECK(job.work_ticks == previous); CHECK(GetUtilityAllocation(rig.assembly->module->consumer)->granted_energy_j == 749);
	}
}

/** A deliberately missing last component is a unit boundary fault; it may neither spend more work nor consume earlier labels or create a product. */
TEST_CASE("Orbital final material admission preserves earlier labels on a missing last component", "[manufacturing-materialization]")
{
	ManufacturingMaterializationRig rig; auto &job = rig.Start(16); rig.Work(GetSpaceVehicleConstruction(16)->work_ticks - 1);
	REQUIRE(HubCargoTransfer::Consume(job.inputs[3], 1)); const auto before = ComputeSpaceStructureDigest(), spent = job.work_energy_j;
	const auto steel = job.inputs[0].TotalCount(), goods = job.inputs[1].TotalCount(); rig.Tick();
	CHECK(job.work_energy_j == spent); CHECK(ComputeSpaceStructureDigest() == before); CHECK(GetSpaceVehicleCount() == 0);
	CHECK(job.inputs[0].TotalCount() == steel); CHECK(job.inputs[1].TotalCount() == goods); CHECK(job.product == SpaceCraftRef{});
}

/** The real company's native unit gate can block the final commit without discarding already paid work or any original input. */
TEST_CASE("Orbital exhausted unit capacity retains completed work for one later materialization", "[manufacturing-materialization]")
{
	ManufacturingMaterializationRig rig; auto &job = rig.Start(19); const auto &recipe = *GetSpaceVehicleConstruction(19);
	{
		AutoRestoreBackup unit_bound(Company::Get(_current_company)->group_all[VehicleType::Space].num_vehicle, static_cast<decltype(Company::Get(_current_company)->group_all[VehicleType::Space].num_vehicle)>(MAX_SPACE_VEHICLES));
		rig.Work(recipe.work_ticks); REQUIRE(job.phase == OrbitalManufacturePhase::AwaitingMaterialization); CHECK(GetSpaceVehicleCount() == 0);
		CHECK(job.work_ticks == recipe.work_ticks); CHECK(job.work_energy_j == uint64_t{recipe.work_ticks} * 750);
		const auto held = job.inputs[1].TotalCount(); const auto energy = job.work_energy_j; rig.Tick(); rig.Tick();
		CHECK(job.phase == OrbitalManufacturePhase::AwaitingMaterialization); CHECK(job.inputs[1].TotalCount() == held); CHECK(job.work_energy_j == energy);
	}
	const auto paid = job.work_energy_j; rig.Tick(); CHECK(job.phase == OrbitalManufacturePhase::Complete); CHECK(GetSpaceVehicleCount() == 1); CHECK(job.work_energy_j == paid);
}

/** Actual native fleet exhaustion retains one fully paid batch; unit-layout positions test an unrelated physical obstruction without inventing a product or commissioning receipt. */
TEST_CASE("Orbital actual fleet and output berth exhaustion preserve one completed batch", "[manufacturing-materialization]")
{
	ManufacturingMaterializationRig rig; SpaceVehicle *first = nullptr;
	// Every capacity occupant is a real product of the same public paid-work/materialization owner. Explicit unit-layout parking is not a native movement/gameplay claim.
	for (uint count = 0; count < MAX_SPACE_VEHICLES; ++count) {
		auto &job = rig.Start(19); rig.Work(GetSpaceVehicleConstruction(19)->work_ticks); REQUIRE(job.phase == OrbitalManufacturePhase::Complete);
		auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
		if (first == nullptr) first = product;
		REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, rig.assembly->index, rig.assembly->generation, rig.core->index, rig.core->generation).Succeeded());
		ReleaseSpaceApproach(*product); product->position = {static_cast<int32_t>(20 + count % 8) * SPACE_Q16, static_cast<int32_t>(20 + count / 8) * SPACE_Q16}; product->UpdateSpacePosition();
	}
	REQUIRE(GetSpaceVehicleCount() == MAX_SPACE_VEHICLES); auto &waiting = rig.Start(19); rig.Work(GetSpaceVehicleConstruction(19)->work_ticks);
	REQUIRE(waiting.phase == OrbitalManufacturePhase::AwaitingMaterialization); const auto paid = waiting.work_energy_j;
	CHECK(waiting.inputs[0].TotalCount() == 2); CHECK(waiting.inputs[1].TotalCount() == 4);
	// Free genuine pool/unit capacity, but place a different existing fully paid hull at the physical output point: neither resource can substitute for a free berth.
	SpaceVehicle *removed = nullptr;
	for (auto *product : SpaceVehicle::Iterate()) if (product != first) { removed = product; break; }
	REQUIRE(removed != nullptr); delete removed;
	first->position = GetSpaceStructureApproach(rig.assembly->state); first->UpdateSpacePosition(); rig.Tick();
	CHECK(waiting.phase == OrbitalManufacturePhase::AwaitingMaterialization); CHECK(waiting.work_energy_j == paid); CHECK(GetSpaceVehicleCount() == MAX_SPACE_VEHICLES - 1);
	first->position = {20 * SPACE_Q16, 20 * SPACE_Q16}; first->UpdateSpacePosition(); rig.Tick();
	CHECK(waiting.phase == OrbitalManufacturePhase::Complete); CHECK(waiting.work_energy_j == paid); CHECK(GetSpaceVehicleCount() == MAX_SPACE_VEHICLES);
	CHECK(waiting.inputs[0].TotalCount() == 0); CHECK(waiting.inputs[1].TotalCount() == 0); CHECK(waiting.product != GetSpaceCraftReference(*first));
}

/** Copied work, final consumption and factory/job provenance reject corruption without an active-world fallback or another materialization. */
TEST_CASE("Orbital completion receipts validate exact saved work and unique factory provenance", "[manufacturing-materialization]")
{
	ManufacturingMaterializationRig rig; auto &job = rig.Start(20); rig.Work(GetSpaceVehicleConstruction(20)->work_ticks);
	const auto rows = CaptureSpaceStructures(); const auto products = CaptureSpaceProduction(); REQUIRE(job.phase == OrbitalManufacturePhase::Complete);
	REQUIRE(ValidateOrbitalManufacturingRecords(rows, GetUtilityPowerInterval()));
	auto bad = rows; --bad[1].module->manufacture->work_energy_j; CHECK_FALSE(ValidateOrbitalManufacturingRecords(bad, GetUtilityPowerInterval()));
	bad = rows; --bad[1].module->manufacture->consumed[1]; CHECK_FALSE(ValidateOrbitalManufacturingRecords(bad, GetUtilityPowerInterval()));
	bad = rows; bad[1].module->manufacture->phase = OrbitalManufacturePhase::AwaitingMaterialization; CHECK_FALSE(ValidateOrbitalManufacturingRecords(bad, GetUtilityPowerInterval()));
	auto forged = products; forged[0].state.origin = {StationID{0}, 1}; CHECK_FALSE(ValidateSpaceProductionRecords(forged, 64, 64));
	forged = products; ++forged[0].state.assembly_serial; CHECK_FALSE(ValidateOrbitalManufacturingProductionLinks(rows, forged, 4, GetNextSpaceVehicleGeneration()));
	forged = products; auto duplicate = forged[0]; duplicate.craft = {VehicleID{500}, duplicate.craft.generation + 1}; forged.push_back(duplicate);
	CHECK_FALSE(ValidateOrbitalManufacturingProductionLinks(rows, forged, 4, GetNextSpaceVehicleGeneration() + 1));
}
