/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_replacement.cpp Native paid replacement admission, work and whole-asset settlement contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12; retained freight fixtures declare physical dimensions before native hauler admission.
 * Frame/packet/age/geometry values are explicit isolated unit inputs; native loaded-world acceptance owns gameplay traffic and payment proof.
 */
#include "../stdafx.h"
#include "processor_freight_fixture.h"
#include "../space_replacement.h"
#include "../space_manifest.h"
#include "../space_purchase.h"
#include "../autoreplace_cmd.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../vehicle_cmd.h"
#include "../group.h"
#include "../group_cmd.h"
#include "../safeguards.h"

/** Real isolated paid producer and original hull, with explicitly declared unit-only old age and reached Core geometry. */
class ManufacturingReplacementRig : public ProcessorFreightRig {
public:
	/** Produce one actual paid source, then admit its actual Core approach and declare reached unit geometry.
	 * @param model Canonical original recipe.
	 * @return Existing old hull, keeping its actual completed birthplace job so replacement must supersede that empty receipt safely.
	 */
	SpaceVehicle &Source(uint16_t model)
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); // The actual approach owner deliberately refuses paused preparation; this isolated reached-geometry fixture must run its real arbitration step.
		auto &job = this->Start(model); this->Work(GetSpaceVehicleConstruction(model)->work_ticks);
		REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *source = ResolveSpaceCraft(job.product); REQUIRE(source != nullptr);
		ReleaseSpaceApproach(*source);
		const SpaceOrderTarget target{SpaceOrderTargetKind::Structure, core->index.base(), core->generation};
		REQUIRE(RequestSpaceApproach(*source, target, true)); PrepareSpaceApproachTick();
		REQUIRE(source->space_orders.approach.has_value()); REQUIRE(source->space_orders.approach->slot != UINT8_MAX);
		source->position = *GetSpaceStructureApproachSlot(*core, source->space_orders.approach->slot); source->UpdateSpacePosition();
		REQUIRE(GetSpaceStructureBerth(*source, *core).has_value());
		source->space_orders.entries = {{SpaceOrderAction::Visit, target}}; source->space_orders.cursor = 1; source->space_orders.blocked = SpaceOrderBlock::Completed;
		source->name = "Retained 07"; source->vehstatus.Reset(VehState::Stopped);
		return *source;
	}
	/** Select a real company rule, retaining native same-model age policy rather than bypassing the replacement command.
	 * @param source Actual old hull.
	 * @param target Canonical existing target role; defining it is explicit isolated catalogue input.
	 */
	void Rule(SpaceVehicle &source, uint16_t target)
	{
		REQUIRE(EnsureSpaceEngineDefinition(target));
		REQUIRE(CmdSetAutoReplace(DoCommandFlag::Execute, source.group_id, source.engine_type, GetSpaceEngine(target), false).Succeeded());
		// Native same-model rules always mean age-driven renewal. This declared unit age exercises that policy without claiming a century of simulated economy.
		source.age = source.max_age + 366;
	}
};

/** Each supported orbital recipe goes through the real native rule/command and one full paid physical job, retiring the old hull only after its final prepared handover. */
TEST_CASE("Space replacement settles each paid native role exactly once", "[manufacturing-replacement]")
{
	// Independent worlds keep each canonical role's recipe, source and native accounting isolated.
	for (uint16_t model : {15, 19, 20, 16, 17}) {
		CAPTURE(model); ManufacturingReplacementRig rig; auto &source = rig.Source(model); rig.Rule(source, model); rig.Supply(model);
		const auto original = GetSpaceCraftReference(source); const auto unit = source.unitnumber; const auto sale = source.value;
		const auto orders = source.space_orders.entries; const auto money = Company::Get(source.owner)->money;
		const auto generation = GetNextSpaceVehicleGeneration();
		auto cost = CmdAutoreplaceVehicle(DoCommandFlag::Execute, source.index); REQUIRE(cost.Succeeded()); CHECK(cost.GetCost() == Money{GetSpaceVehicleConstruction(model)->capital});
		REQUIRE(rig.assembly->module->manufacture != nullptr); auto &job = *rig.assembly->module->manufacture;
		REQUIRE(job.replacement.has_value()); CHECK(job.replacement->source == original); CHECK_FALSE(job.replacement->settled); CHECK(job.replacement->resume);
		CHECK(source.vehstatus.Test(VehState::Stopped)); CHECK(GetSpaceVehicleCount() == 1); CHECK(GetNextSpaceVehicleGeneration() == generation);
		CHECK(CmdAutoreplaceVehicle(DoCommandFlag::Execute, source.index).Failed());
		rig.Tick(); rig.Work(GetSpaceVehicleConstruction(model)->work_ticks - 1);
		CHECK(ResolveSpaceCraft(original) == &source); CHECK(job.product == SpaceCraftRef{}); CHECK_FALSE(job.replacement->settled);
		rig.Tick(); REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
		CHECK(ResolveSpaceCraft(original) == nullptr); CHECK(GetSpaceVehicleCount() == 1); CHECK(product->unitnumber == unit); CHECK(product->name == "Retained 07");
		CHECK(product->space_orders.entries == orders); CHECK(product->space_orders.cursor == 1); CHECK_FALSE(product->vehstatus.Test(VehState::Stopped));
		CHECK(GetSpaceStructureBerth(*product, *rig.assembly).has_value()); CHECK(job.replacement->settled); CHECK(job.replacement->sale_credit == static_cast<uint32_t>(sale));
		// Direct unit commands quote capital without top-level accounting; the synchronous real native retirement credits its observed sale exactly once.
		CHECK(Company::Get(product->owner)->money == money + sale);
		const auto digest = ComputeSpaceStructureDigest(); PreparedSpaceReplacement empty;
		CommitSpaceReplacement(*job.replacement, *product, std::move(empty)); rig.Tick();
		CHECK(ComputeSpaceStructureDigest() == digest); CHECK(Company::Get(product->owner)->money == money + sale); CHECK(GetSpaceVehicleCount() == 1);
		CHECK(ValidateSpaceProduction());
	}
}

/** Current original assets and later legal configuration edits survive; a carried body, active work or incompatible whole holding keeps every paid input and the old hull for retry. */
TEST_CASE("Space replacement preserves current whole assets and waits on unsafe work", "[manufacturing-replacement]")
{
	ManufacturingReplacementRig rig; auto &source = rig.Source(20); rig.Rule(source, 20); rig.Supply(20);
	const auto original = GetSpaceCraftReference(source);
	REQUIRE(CmdAutoreplaceVehicle(DoCommandFlag::Execute, source.index).Succeeded()); auto &job = *rig.assembly->module->manufacture;
	auto &manifest = GetOrCreateSpaceManifest(source); REQUIRE(GetMultiLayerSpaceBudget().returns_per_service > 0);
	const auto used_allowance = static_cast<uint8_t>(GetMultiLayerSpaceBudget().returns_per_service - 1); manifest.returns_remaining = used_allowance; manifest.services_completed = 2;
	source.name = "Later retained"; source.space_orders.cursor = 0;
	const auto cargo = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	AutoRestoreBackup weight(CargoSpec::Get(cargo)->weight, uint8_t{16}); // Explicit isolated native cargo dimensions, matching the declared unit packet; this is not a loaded-package mass acceptance claim.
	source.cargo_type = cargo; source.cargo_cap = GetSpaceFreightCapacity(20, cargo);
	REQUIRE(CargoPacket::CanAllocateItem()); auto *packet = CargoPacket::Create(uint16_t{1}, uint16_t{7}, StationID::Invalid(), source.tile, Money{11});
	const auto packet_id = packet->index; source.cargo.Append(packet);
	REQUIRE(CargoPacket::CanAllocateItem()); manifest.supplies[1].Append(CargoPacket::Create(uint16_t{1}, uint16_t{9}, StationID::Invalid(), source.tile, Money{17}), StationID::Invalid());
	rig.Tick();
	// Active old action progress is a late refusal, after admission; the fully earned paid milestone must retain all four physical owners until that work is actually released.
	source.space_orders.started = true; source.space_orders.work_ticks = 1;
	rig.Work(GetSpaceVehicleConstruction(20)->work_ticks);
	CHECK(job.phase == OrbitalManufacturePhase::AwaitingMaterialization); CHECK_FALSE(job.replacement->settled); CHECK(ResolveSpaceCraft(original) == &source);
	CHECK(job.inputs[0].TotalCount() == GetSpaceVehicleConstruction(20)->inputs[0].quantity); CHECK(source.cargo.TotalCount() == 1);
	source.space_orders.started = false; source.space_orders.work_ticks = 0; rig.Tick();
	REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
	CHECK(product->name == "Later retained"); CHECK(product->space_orders.cursor == 0); CHECK(product->cargo.TotalCount() == 1); CHECK(CargoPacket::Get(packet_id)->GetFeederShare() == Money{11});
	REQUIRE(product->manifest != nullptr); CHECK(product->manifest->returns_remaining == used_allowance); CHECK(product->manifest->services_completed == 2); CHECK(product->manifest->supplies[1].TotalCount() == 1);
}

/** Pure quotes and cancellations retain complete actual owners, including the native requested running state; no projected sale can fund a new full recipe. */
TEST_CASE("Space replacement quotes and cancellation preserve original owners", "[manufacturing-replacement]")
{
	ManufacturingReplacementRig rig; auto &source = rig.Source(15); rig.Rule(source, 15); rig.Supply(15);
	const auto structures = ComputeSpaceStructureDigest(), generation = GetNextSpaceVehicleGeneration(); const auto money = Company::Get(source.owner)->money; const auto random = _random;
	// Every inspection uses the same actual native policy and physical producer; mixed flags must remain observational too.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::QueryCost}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) {
		CHECK(CmdAutoreplaceVehicle(flags, source.index).Succeeded()); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(GetNextSpaceVehicleGeneration() == generation);
		CHECK(Company::Get(source.owner)->money == money); CHECK_FALSE(source.vehstatus.Test(VehState::Stopped)); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	}
	REQUIRE(CmdAutoreplaceVehicle(DoCommandFlag::Execute, source.index).Succeeded()); rig.Tick(); rig.Work(3);
	const auto before = ComputeSpaceStructureDigest();
	CHECK(CmdCancelOrbitalManufacture({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, rig.assembly->index, rig.assembly->generation, rig.core->index, rig.core->generation).Succeeded()); CHECK(ComputeSpaceStructureDigest() == before);
	CHECK(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, rig.assembly->index, rig.assembly->generation, rig.core->index, rig.core->generation).Succeeded());
	CHECK(rig.assembly->module->manufacture == nullptr); CHECK_FALSE(source.vehstatus.Test(VehState::Stopped)); CHECK(Company::Get(source.owner)->money == money);
	CHECK(GetSpaceVehicleCount() == 1); CHECK(GetNextSpaceVehicleGeneration() == generation);
	Company::Get(source.owner)->money = 0; CHECK(CmdAutoreplaceVehicle(DoCommandFlag::Execute, source.index).Failed()); CHECK(GetSpaceVehicleCount() == 1);
}

/** Existing native group/profit/unit owners transfer once, while a genuine capacity refusal retains the original job and the exact depreciated final sale value. */
TEST_CASE("Space replacement preserves native group statistics through capacity retry", "[manufacturing-replacement]")
{
	ManufacturingReplacementRig rig; auto &source = rig.Source(19);
	auto [created, group] = CmdCreateGroup(DoCommandFlag::Execute, VehicleType::Space, GroupID::Invalid()); REQUIRE(created.Succeeded());
	auto [assigned, actual_group] = CmdAddVehicleGroup(DoCommandFlag::Execute, group, source.index, false, VehicleListIdentifier{}); REQUIRE(assigned.Succeeded()); REQUIRE(actual_group == group);
	// Explicit isolated historical profit/value inputs exercise the existing accounting owners; they do not claim earned native traffic income or elapsed depreciation.
	source.profit_last_year = Money{12} * 256; source.profit_this_year = Money{34} * 256; GroupStatistics::AddProfitLastYear(&source); source.value /= 2;
	const auto old = GetSpaceCraftReference(source); const auto unit = source.unitnumber; const auto sale = source.value; const auto owner = source.owner;
	rig.Rule(source, 19); rig.Supply(19); REQUIRE(CmdAutoreplaceVehicle(DoCommandFlag::Execute, source.index).Succeeded()); auto &job = *rig.assembly->module->manufacture; rig.Tick();
	{
		AutoRestoreBackup unit_bound(Company::Get(owner)->group_all[VehicleType::Space].num_vehicle, static_cast<uint16_t>(MAX_SPACE_VEHICLES));
		rig.Work(GetSpaceVehicleConstruction(19)->work_ticks); CHECK(job.phase == OrbitalManufacturePhase::AwaitingMaterialization); CHECK(ResolveSpaceCraft(old) == &source); CHECK_FALSE(job.replacement->settled);
		CHECK(job.inputs[0].TotalCount() == GetSpaceVehicleConstruction(19)->inputs[0].quantity);
	}
	rig.Tick(); REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
	CHECK(product->group_id == group); CHECK(product->unitnumber == unit); CHECK(product->profit_last_year == Money{12} * 256); CHECK(product->profit_this_year == Money{34} * 256);
	CHECK(Group::Get(group)->statistics.num_vehicle == 1); CHECK(Group::Get(group)->statistics.profit_last_year == Money{12});
	CHECK(GroupStatistics::GetAllGroup(product).num_vehicle == 1); CHECK(GroupStatistics::GetAllGroup(product).GetNumEngines(product->engine_type) == 1); CHECK(job.replacement->sale_credit == static_cast<uint32_t>(sale));
	REQUIRE(CmdDeleteGroup(DoCommandFlag::Execute, group).Succeeded()); // Retire this isolated native test owner before the surrounding company's fixture is torn down.
}

/** Actual native Engine startup makes a newly registered paid role usable by company replacement policy without consuming random simulation state or resetting retained catalogue history. */
TEST_CASE("Space replacement uses native Engine availability without resetting history", "[manufacturing-replacement]")
{
	ManufacturingMaterializationRig rig; const auto random = _random; const auto generation = GetNextSpaceVehicleGeneration(); const auto money = Company::Get(_current_company)->money;
	REQUIRE(EnsureSpaceEngineDefinition(16)); auto *engine = Engine::Get(GetSpaceEngine(16));
	CHECK(IsEngineBuildable(engine->index, VehicleType::Space, _current_company)); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(Company::Get(_current_company)->money == money); CHECK(GetSpaceVehicleCount() == 0);
	const auto availability = engine->company_avail; const auto flags = engine->flags; const auto age = engine->age; const auto reliability = engine->reliability;
	REQUIRE(EnsureSpaceEngineDefinition(16)); CHECK(engine->company_avail == availability); CHECK(engine->flags == flags); CHECK(engine->age == age); CHECK(engine->reliability == reliability);
}

/** A later physically loaded builder packet cannot become another role's mined freight merely because it fits by mass; unloading preserves that packet and permits the already paid handover. */
TEST_CASE("Space replacement waits for incompatible whole freight without relabeling it", "[manufacturing-replacement]")
{
	ManufacturingReplacementRig rig; auto &source = rig.Source(20); rig.Rule(source, 15); rig.Supply(15); const auto old = GetSpaceCraftReference(source);
	REQUIRE(CmdAutoreplaceVehicle(DoCommandFlag::Execute, source.index).Succeeded()); auto &job = *rig.assembly->module->manufacture; rig.Tick();
	const auto cargo = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	AutoRestoreBackup weight(CargoSpec::Get(cargo)->weight, uint8_t{16}); // Declare this isolated packet's physical mass before using the genuine native load/unload gate.
	rig.Add(*rig.core, CargoLabel{"GOOD"}, 1);
	REQUIRE(CmdMoveOrbitalFreight(DoCommandFlag::Execute, rig.core->index, rig.core->generation, source.index, source.generation, cargo, 1, true).Succeeded());
	rig.Work(GetSpaceVehicleConstruction(15)->work_ticks); CHECK(job.phase == OrbitalManufacturePhase::AwaitingMaterialization); CHECK(source.cargo.TotalCount() == 1);
	CHECK(ResolveSpaceCraft(old) == &source); CHECK_FALSE(job.replacement->settled); CHECK(job.inputs[0].TotalCount() == GetSpaceVehicleConstruction(15)->inputs[0].quantity);
	REQUIRE(CmdMoveOrbitalFreight(DoCommandFlag::Execute, rig.core->index, rig.core->generation, source.index, source.generation, cargo, 1, false).Succeeded());
	CHECK(rig.core->module->stock[cargo].TotalCount() == 1); rig.Tick(); REQUIRE(job.phase == OrbitalManufacturePhase::Complete);
	CHECK(ResolveSpaceCraft(old) == nullptr); CHECK(ResolveSpaceCraft(job.product)->cargo.TotalCount() == 0); CHECK(rig.core->module->stock[cargo].TotalCount() == 1);
}
