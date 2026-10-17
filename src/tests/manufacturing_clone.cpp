/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_clone.cpp Isolated paid clone intent, source ownership and prepared native completion contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BB
 * Frames, packet content and explicit geometry are unit inputs; separate loaded-world observers own gameplay payment/traffic acceptance.
 */
#include "../stdafx.h"
#include "mock_environment.h"
#include "manufacturing_fixture.h"
#include "../space_clone.h"
#include "../space_manifest.h"
#include "../vehicle_cmd.h"
#include "../engine_base.h"
#include "../safeguards.h"

/** Actual isolated first product followed by a distinct full recipe; geometric relocation below is explicitly unit setup, never gameplay navigation proof. */
class ManufacturingCloneRig : public ManufacturingMaterializationRig {
public:
	/** Materialize one real first product, acknowledge its completed job and prepare independent source instructions plus a separate full bill.
	 * @param model Canonical model whose actual native Engine/receipt is supplied by the first completed producer.
	 * @return Actual original source hull, with a completed instruction to distinguish copied entries from copied action progress.
	 */
	SpaceVehicle &Source(uint16_t model)
	{
		auto &job = this->Start(model); this->Work(GetSpaceVehicleConstruction(model)->work_ticks);
		REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *source = ResolveSpaceCraft(job.product); REQUIRE(source != nullptr);
		REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, assembly->index, assembly->generation, core->index, core->generation).Succeeded());
		ReleaseSpaceApproach(*source);
		source->position = {30 * SPACE_Q16, 30 * SPACE_Q16}; source->UpdateSpacePosition(); source->vehstatus.Set(VehState::Stopped);
		source->name = "Fleet 07";
		source->space_orders.entries = {{SpaceOrderAction::Visit, {SpaceOrderTargetKind::Structure, core->index.base(), core->generation}}};
		source->space_orders.cursor = 1; source->space_orders.blocked = SpaceOrderBlock::Completed;
		this->Supply(model); return *source;
	}
	/** @return Actual orbital Assembly world anchor used by the ordinary clone command. */
	TileIndex Tile() const { return TileXYL(assembly->state.x, assembly->state.y, MapLayer::Space); }
	/** @param source Actual original paid source. @return Its newly admitted actual contained paid clone job, with no product yet. */
	OrbitalManufactureJob &Clone(const SpaceVehicle &source)
	{
		auto [cost, id] = CmdCloneVehicle(DoCommandFlag::Execute, this->Tile(), source.index, false);
		REQUIRE(cost.Succeeded()); CHECK(cost.GetCost() == Money{GetSpaceVehicleConstruction(GetSpaceVehicleModel(source))->capital}); CHECK(id == VehicleID::Invalid());
		REQUIRE(assembly->module->manufacture != nullptr); REQUIRE(assembly->module->manufacture->clone.has_value());
		return *assembly->module->manufacture;
	}
};

/** Each original native role requests a new full physical batch and receives one independent stopped product, never the original action progress or berth. */
TEST_CASE("Ordinary Space clones manufacture independent captured instructions", "[manufacturing-clone]")
{
	// Independent worlds ensure each role supplies its own first paid source and separate full new bill rather than borrowing another model's identity/work.
	for (uint16_t model : {15, 19, 20, 16, 17}) {
		CAPTURE(model); ManufacturingCloneRig rig; auto &source = rig.Source(model); const auto original = GetSpaceCraftReference(source);
		const auto orders = source.space_orders; auto &job = rig.Clone(source); const auto &recipe = *GetSpaceVehicleConstruction(model);
		CHECK(job.clone->source == original); CHECK(job.clone->name == source.name); CHECK(job.clone->orders == orders.entries); CHECK(GetSpaceVehicleCount() == 1);
		rig.Tick(); CHECK(job.phase == OrbitalManufacturePhase::Reserved); rig.Work(recipe.work_ticks);
		REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
		CHECK(GetSpaceVehicleCount() == 2); CHECK(product != &source); CHECK(product->name == "Fleet 08"); CHECK(product->vehstatus.Test(VehState::Stopped));
		CHECK(product->space_orders.entries == orders.entries); CHECK(product->space_orders.cursor == 0); CHECK_FALSE(product->space_orders.started);
		CHECK(product->space_orders.timer_ticks == 0); CHECK(product->space_orders.work_ticks == 0); CHECK(product->space_orders.blocked == SpaceOrderBlock::None);
		CHECK(GetSpaceStructureBerth(*product, *rig.assembly) == std::optional<uint8_t>{0}); CHECK(source.space_orders == orders);
		CHECK(ResolveSpaceCraft(original) == &source); CHECK(product->construction.assembly_serial == job.serial);
		const auto digest = ComputeSpaceStructureDigest(); rig.Tick(); rig.Tick(); CHECK(job.product == GetSpaceCraftReference(*product)); CHECK(GetSpaceVehicleCount() == 2);
		// Repeated normal opportunities cannot replay the completed material bill, captured name or instructions.
		CHECK(ComputeSpaceStructureDigest() == digest);
	}
}

/** Captured instructions/name remain fixed while the real source retains its own physical freight, service packets and previously used allowance. */
TEST_CASE("Space clone intent never transfers original hull assets or mutable configuration", "[manufacturing-clone]")
{
	ManufacturingCloneRig rig; auto &source = rig.Source(15);
	// Explicit unit cargo dimensions/packets characterize ownership; native freight/service acceptance must use the actual paid transport producers.
	source.cargo_type = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	CargoSpec::Get(source.cargo_type)->weight = 16; source.cargo_cap = 8;
	REQUIRE(CargoPacket::CanAllocateItem());
	source.cargo.Append(CargoPacket::Create(uint16_t{2}, uint16_t{7}, StationID::Invalid(), source.tile, Money{20}));
	auto &manifest = GetOrCreateSpaceManifest(source); manifest.returns_remaining = 3;
	REQUIRE(CargoPacket::CanAllocateItem());
	manifest.supplies[1].Append(CargoPacket::Create(uint16_t{1}, uint16_t{4}, StationID::Invalid(), source.tile, Money{10}), StationID::Invalid());
	const auto entries = source.space_orders.entries; auto &job = rig.Clone(source);
	source.name = "Changed 55"; source.space_orders = {};
	rig.Tick(); rig.Work(GetSpaceVehicleConstruction(15)->work_ticks);
	auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr); REQUIRE(product->manifest != nullptr);
	CHECK(source.cargo.TotalCount() == 2); CHECK(manifest.supplies[1].TotalCount() == 1); CHECK(manifest.returns_remaining == 3);
	CHECK(product->cargo.TotalCount() == 0); CHECK(product->manifest->supplies[1].TotalCount() == 0);
	CHECK(source.cargo_type == GetCargoTypeByLabel(CargoLabel{"GOOD"})); CHECK(product->cargo_type == product->GetEngine()->GetDefaultCargoType());
	CHECK(product->manifest->returns_remaining == GetMultiLayerSpaceBudget().returns_per_service);
	CHECK(product->name == "Fleet 08"); CHECK(product->space_orders.entries == entries); CHECK(source.name == "Changed 55"); CHECK(source.space_orders.entries.empty());
}

/** Source deletion/numeric reuse and cancellation operate on the captured job only, preserving once-only real work and original packet owners. */
TEST_CASE("Space clone survives original identity loss or cancels its own batch", "[manufacturing-clone]")
{
	ManufacturingCloneRig rig; auto &source = rig.Source(16); const auto original = GetSpaceCraftReference(source); auto &job = rig.Clone(source);
	rig.Tick(); rig.Work(7);
	SECTION("Cancellation returns only the new unconsumed bill") {
		REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, rig.assembly->index, rig.assembly->generation, rig.core->index, rig.core->generation).Succeeded());
		CHECK(rig.assembly->module->manufacture == nullptr); CHECK(ResolveSpaceCraft(original) == &source); CHECK(GetSpaceVehicleCount() == 1);
		// Every component remains the same actual returned quantity; the source's embodied first recipe is never unbuilt or refunded.
		for (const auto &input : GetSpaceVehicleConstruction(16)->inputs) CHECK(rig.core->module->stock[GetCargoTypeByLabel(input.label)].TotalCount() == input.quantity);
	}
	SECTION("The new actual product may reuse a deleted source ID with a new incarnation") {
		delete &source;
		CHECK(ResolveSpaceCraft(original) == nullptr); CHECK(job.clone->source == original);
		rig.Work(GetSpaceVehicleConstruction(16)->work_ticks - job.work_ticks);
		auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr); CHECK(product->index == original.vehicle); CHECK(product->generation != original.generation);
		CHECK(job.clone->source == original); CHECK(product->name == "Fleet 08"); CHECK(GetSpaceVehicleCount() == 1);
		CHECK(ValidateOrbitalManufacturingProductionLinks(CaptureSpaceStructures(), CaptureSpaceProduction(), GetNextSpaceStructureGeneration(), GetNextSpaceVehicleGeneration()));
		auto forged = CaptureSpaceStructures(); forged[1].module->manufacture->clone->source = job.product;
		CHECK_FALSE(ValidateOrbitalManufacturingProductionLinks(forged, CaptureSpaceProduction(), GetNextSpaceStructureGeneration(), GetNextSpaceVehicleGeneration()));
	}
	SECTION("Source ownership changes do not transfer its assets or retarget captured intent") {
		REQUIRE(Company::CanAllocateItem()); auto *other = Company::Create();
		{
			AutoRestoreBackup owner(source.owner, other->index); source.name = "Other fleet 55";
			rig.Work(GetSpaceVehicleConstruction(16)->work_ticks - job.work_ticks);
			auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
			CHECK(product->owner == rig.assembly->state.owner); CHECK(source.owner == other->index); CHECK(product->name == "Fleet 08"); CHECK(job.clone->source == original);
		}
		delete other;
	}
}

/** Late target ownership loss cannot commit another company's actionable queue; restored rights retry the already paid final milestone without more work. */
TEST_CASE("Space clone completion waits for surviving typed target rights", "[manufacturing-clone]")
{
	ManufacturingCloneRig rig; auto &source = rig.Source(19);
	source.space_orders.entries = {{SpaceOrderAction::Visit, {SpaceOrderTargetKind::Structure, rig.storage->index.base(), rig.storage->generation}}};
	auto &job = rig.Clone(source); rig.Tick(); rig.Work(GetSpaceVehicleConstruction(19)->work_ticks - 1);
	REQUIRE(Company::CanAllocateItem()); auto *other = Company::Create();
	{
		AutoRestoreBackup owner(rig.storage->state.owner, other->index); rig.Tick();
		CHECK(job.phase == OrbitalManufacturePhase::AwaitingMaterialization); CHECK(GetSpaceVehicleCount() == 1); CHECK(job.inputs[0].TotalCount() == 2); CHECK(job.inputs[1].TotalCount() == 4);
	}
	delete other; const auto paid = job.work_energy_j; rig.Tick();
	CHECK(job.phase == OrbitalManufacturePhase::Complete); CHECK(job.work_energy_j == paid); CHECK(GetSpaceVehicleCount() == 2);
}

/** Inspection and unsupported source/destination configuration preserve all original native job, packet, financial and random owners. */
TEST_CASE("Space clone quotes and refusals cannot create temporary hulls", "[manufacturing-clone]")
{
	ManufacturingCloneRig rig; auto &source = rig.Source(15); DoCommandFlags flags{}; bool share = false; auto tile = rig.Tile(); bool accepted = false;
	SECTION("Ordinary quote") { accepted = true; }
	SECTION("Mixed execution and query flags remain pure") { flags = {DoCommandFlag::Execute, DoCommandFlag::QueryCost}; accepted = true; }
	SECTION("Sharing requires a real shared-list owner") { share = true; flags.Set(DoCommandFlag::Execute); }
	SECTION("A dynamic source freight label remains with its original cargo") { source.cargo_type = GetCargoTypeByLabel(CargoLabel{"GOOD"}); accepted = true; }
	SECTION("An invalid actual cargo identity cannot authorize a clone") { source.cargo_type = INVALID_CARGO; }
	SECTION("Unsupported subtype cannot bypass native configuration") { source.cargo_subtype = 1; }
	SECTION("A foreign source cannot grant clone permission") { source.owner = OWNER_NONE; }
	SECTION("A legacy demonstration hull is not a paid source") { source.construction.capital_paid = 0; }
	SECTION("An absent incarnation cannot name captured history") { source.generation = 0; }
	SECTION("Invalid facility refuses before any build") { tile = INVALID_TILE; }
	SECTION("Funds refuse NoTest before a physical mutation") { Company::Get(_current_company)->money = 0; flags.Set(DoCommandFlag::Execute); }
	const auto structures = ComputeSpaceStructureDigest(), generation = GetNextSpaceVehicleGeneration(), utility = ComputeUtilityPowerDigest(); const auto random = _random;
	const auto money = Company::Get(_current_company)->money; auto [cost, id] = CmdCloneVehicle(flags, tile, source.index, share);
	CHECK(cost.Succeeded() == accepted); CHECK(id == VehicleID::Invalid()); CHECK(GetSpaceVehicleCount() == 1); CHECK(rig.assembly->module->manufacture == nullptr);
	CHECK(ComputeSpaceStructureDigest() == structures); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(Company::Get(_current_company)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Detached history is bounded against copied issuance and native role policy; legal naming cannot overflow or create an oversized custom field. */
TEST_CASE("Space clone copied intent and ordinary unique naming are bounded", "[manufacturing-clone]")
{
	ManufacturingCloneRig rig; auto &source = rig.Source(15); const auto next = GetNextSpaceVehicleGeneration(), structures = GetNextSpaceStructureGeneration();
	SpaceCloneIntent intent{GetSpaceCraftReference(source), source.name, source.space_orders.entries}; bool accepted = false; uint16_t model = 15;
	SECTION("Issued original intent") { accepted = true; }
	SECTION("Future source") { intent.source.generation = next; }
	SECTION("Invalid original pool ID") { intent.source.vehicle = VehicleID::Invalid(); }
	SECTION("Future static target") { intent.orders[0].target.generation = structures; }
	SECTION("Future craft target") { intent.orders[0].target = {SpaceOrderTargetKind::Craft, source.index.base(), next}; }
	SECTION("A passive shell is not a native clone role") { model = 18; intent.orders.clear(); }
	SECTION("Unknown action") { intent.orders[0].action = SpaceOrderAction::End; }
	SECTION("Too many instructions") { const auto order = intent.orders[0]; intent.orders.resize(MAX_SPACE_ORDERS + 1, order); }
	SECTION("Overlong captured text") { intent.name.assign(MAX_SPACE_CLONE_NAME_BYTES + 1, 'x'); }
	SECTION("Embedded NUL") { intent.name = std::string("a\0b", 3); }
	SECTION("Invalid UTF-8") { intent.name = "\xFF"; }
	SECTION("Maximum legal UTF-8 text and independent instruction count") {
		intent.name.clear();
		// Four-byte printable codepoints reach the byte maximum while retaining the ordinary31-character custom-name limit.
		for (uint count = 0; count < MAX_LENGTH_VEHICLE_NAME_CHARS - 1; ++count) intent.name += "\xF0\x9F\x9A\x80";
		// Preserve the independent value before replacing vector storage; the bounds fixture must remain legal under checked native container implementations too.
		const auto order = intent.orders[0]; intent.orders.assign(MAX_SPACE_ORDERS, order); accepted = true;
	}
	CHECK(ValidateSpaceCloneIntent(intent, model, next, structures) == accepted);
	CHECK(MakeClonedVehicleName("").empty()); CHECK(MakeClonedVehicleName("Fleet 07") == "Fleet 08");
	CHECK(MakeClonedVehicleName("Fleet") == "Fleet 2"); CHECK(MakeClonedVehicleName("18446744073709551615").empty());
	CHECK(MakeClonedVehicleName("9999999999999999999999999999999").empty()); CHECK(MakeClonedVehicleName(std::string(31, 'x')).empty());
	source.name = "Fleet 08"; CHECK(MakeClonedVehicleName("Fleet 07") == "Fleet 09");
}
