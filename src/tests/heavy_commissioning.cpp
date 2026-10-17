/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_commissioning.cpp Exact legacy content identity, mixed paid provenance and once-only heavy factory boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9B
 * Factory frames, funds and packets are declared isolated unit inputs; actual procurement/flight is verified by the separate loaded-world acceptance.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_mining_tuning.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../engine_base.h"
#include "../space_clone.h"
#include "../space_replacement.h"
#include "../space_purchase.h"
#include "../autoreplace_cmd.h"
#include "../vehicle_cmd.h"
#include "../core/random_func.hpp"
#include "manufacturing_fixture.h"
#include "../safeguards.h"

/** The old fingerprint is copied from actual accepted429 SDEF bytes, independently of today's hash implementation or appended tables. */
TEST_CASE("Space content3 preserves the exact original paid content2 fingerprint", "[heavy-commissioning]")
{
	constexpr uint64_t original = 0xa1608eff08bf795bULL; // Accepted replacement-complete-19-429.sav, SHA256 b0cb661cdbb85c4c99781c8d6b8966c5c7444cc446e7da977ed7e480264438f7.
	CHECK(GetMultiLayerSpaceContentSignatureForVersion(2) == original);
	CHECK(GetMultiLayerSpaceContentSignature() != original); CHECK(GetMultiLayerSpaceContentSignatureForVersion(1) == 0);
	CHECK(IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID::MultilayerIndustry, 2, original));
	CHECK(IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID::MultilayerIndustry, 3, GetMultiLayerSpaceContentSignature()));
	CHECK_FALSE(IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID::MultilayerIndustry, 2, GetMultiLayerSpaceContentSignature()));
	CHECK(IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID::Classic, 0, 0));
	CHECK_FALSE(IsSupportedMultiLayerSpaceContent(MultiLayerGameplayProfileID::Classic, 3, GetMultiLayerSpaceContentSignature()));
}

/** New files may preserve genuine older paid receipts without letting a new role borrow their provenance or Earth bootstrap. */
TEST_CASE("Appended miner provenance requires3 while existing receipts retain2", "[heavy-commissioning]")
{
	// Every existing hull/payload recipe remains eligible for its original receipt version; data support never substitutes for full native producer validation.
	for (uint16_t model : {14, 15, 16, 17, 18, 19, 20}) {
		CHECK(IsSupportedSpaceVehicleContentVersion(2, model)); CHECK(IsSupportedSpaceVehicleContentVersion(3, model));
	}
	CHECK_FALSE(IsSupportedSpaceVehicleContentVersion(2, SPACE_HEAVY_MINER_MODEL));
	CHECK(IsSupportedSpaceVehicleContentVersion(3, SPACE_HEAVY_MINER_MODEL));
	CHECK_FALSE(IsSupportedSpaceVehicleContentVersion(0, SPACE_HEAVY_MINER_MODEL)); CHECK_FALSE(IsSupportedSpaceVehicleContentVersion(3, 22));
	CHECK_FALSE(CanCommissionSpaceVehicle(SPACE_HEAVY_MINER_MODEL, MultiLayerCommissioningFacility::RocketPayload, true));
	CHECK_FALSE(CanCommissionSpaceVehicle(SPACE_HEAVY_MINER_MODEL, MultiLayerCommissioningFacility::LaunchCenter, true));
	CHECK_FALSE(CanCommissionSpaceVehicle(SPACE_HEAVY_MINER_MODEL, MultiLayerCommissioningFacility::OrbitalAssembly, false));
	CHECK(CanCommissionSpaceVehicle(SPACE_HEAVY_MINER_MODEL, MultiLayerCommissioningFacility::OrbitalAssembly, true));
}

/** Heavy quote/reservation/full-grant work create one real native body, consume all four original inputs once and grant no physical pod or loose service packet. */
TEST_CASE("Heavy miner materialization retains paid inputs through the original final opportunity", "[heavy-commissioning]")
{
	ManufacturingMaterializationRig rig; const auto *recipe = GetSpaceVehicleConstruction(SPACE_HEAVY_MINER_MODEL); REQUIRE(recipe != nullptr);
	AutoRestoreBackup weight(CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"NORE"}))->weight, uint8_t{16}); // Explicit one-tonne ore unit dimension; the base isolated fixture installs labels with zero weights.
	const auto engine = GetSpaceEngine(SPACE_HEAVY_MINER_MODEL); const auto generation = GetNextSpaceVehicleGeneration();
	const auto money = Company::Get(_current_company)->money; const auto random = _random;
	rig.Supply(SPACE_HEAVY_MINER_MODEL); // Declared full isolated bill; even a pure quote must pass the original real-stock admission gate.
	REQUIRE(rig.Queue(SPACE_HEAVY_MINER_MODEL).Succeeded()); CHECK(Company::Get(_current_company)->money == money);
	CHECK(GetSpaceEngine(SPACE_HEAVY_MINER_MODEL) == engine); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(GetSpaceVehicleCount() == 0);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	REQUIRE(rig.Queue(SPACE_HEAVY_MINER_MODEL, DoCommandFlag::Execute).Succeeded()); rig.Tick();
	auto &job = *rig.assembly->module->manufacture; REQUIRE(job.phase == OrbitalManufacturePhase::Reserved); CHECK(job.work_ticks == 0); CHECK(job.capital_paid == recipe->capital);
	// Direct isolated command calls preserve accounting; the loaded-world Command::Post acceptance separately proves the real company capital debit.
	CHECK(Company::Get(_current_company)->money == money);
	const auto energy = GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j;
	rig.Work(recipe->work_ticks - 1); CHECK(job.work_ticks == recipe->work_ticks - 1); CHECK(GetSpaceVehicleCount() == 0);
	// All four real reserved labels survive partial work; an absent output hull cannot claim an early final material consumption.
	for (size_t slot = 0; slot < recipe->inputs.size(); ++slot) CHECK(job.inputs[slot].TotalCount() == recipe->inputs[slot].quantity);
	rig.Work(1); REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
	CHECK(GetSpaceVehicleModel(*product) == SPACE_HEAVY_MINER_MODEL); CHECK(product->construction.content_version == 3); CHECK(product->cargo_cap == 40);
	REQUIRE(product->manifest != nullptr); CHECK(product->manifest->returns_remaining == 2); CHECK(product->manifest->services_completed == 0); CHECK(product->manifest->pods.empty());
	CHECK(product->manifest->supplies[1].TotalCount() == 0); CHECK(product->manifest->supplies[2].TotalCount() == 0);
	CHECK(product->GetRunningCost() == Money{200} * CalendarTime::DAYS_IN_YEAR * 256); CHECK(product->GetEngine()->GetRunningCost() == Money{200} * CalendarTime::DAYS_IN_YEAR);
	CHECK(GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j == energy + uint64_t{recipe->work_ticks} * 750);
	// Final consumption retains one complete four-label material ledger; clearing source packets cannot erase the actual embodied bill.
	for (size_t slot = 0; slot < recipe->inputs.size(); ++slot) { CHECK(job.inputs[slot].TotalCount() == 0); CHECK(job.consumed[slot] == recipe->inputs[slot].quantity); }
	const auto original = job.product; const auto digest = ComputeSpaceStructureDigest();
	{ AutoRestoreBackup pause(_pause_mode, PauseModes{}); TickOrbitalManufacturingWork(); TickOrbitalManufacturingWork(); }
	CHECK(job.product == original); CHECK(GetSpaceVehicleCount() == 1); CHECK(ComputeSpaceStructureDigest() == digest);
	auto receipts = CaptureSpaceProduction(); REQUIRE(receipts.size() == 1); receipts.front().state.content_version = 2;
	CHECK_FALSE(ValidateSpaceProductionRecords(receipts, 64, 64)); CHECK(product->construction.content_version == 3);
}

/** Isolated separately paid heavy source and second bill; reached berth/age are declared unit inputs, never claimed as navigation or procurement evidence. */
class HeavyCommissioningRig : public ManufacturingMaterializationRig {
	AutoRestoreBackup<uint8_t> ore_weight{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"NORE"}))->weight, uint8_t{16}}; ///< Explicit isolated one-tonne ore dimension, restored before the parent cargo registry.
public:
	/** Materialize the original paid heavy hull, then use the original approach authority with explicitly reached unit geometry.
	 * @return Original native21 body, independently owned from its empty completed factory receipt.
	 */
	SpaceVehicle &Source()
	{
		auto &job = this->Start(SPACE_HEAVY_MINER_MODEL); this->Work(GetSpaceVehicleConstruction(SPACE_HEAVY_MINER_MODEL)->work_ticks);
		auto *source = ResolveSpaceCraft(job.product); REQUIRE(source != nullptr);
		REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, assembly->index, assembly->generation, core->index, core->generation).Succeeded());
		ReleaseSpaceApproach(*source);
		AutoRestoreBackup pause(_pause_mode, PauseModes{});
		REQUIRE(RequestSpaceApproach(*source, {SpaceOrderTargetKind::Structure, core->index.base(), core->generation}, true)); PrepareSpaceApproachTick();
		REQUIRE(source->space_orders.approach.has_value()); REQUIRE(source->space_orders.approach->slot != UINT8_MAX);
		source->position = *GetSpaceStructureApproachSlot(*core, source->space_orders.approach->slot); source->UpdateSpacePosition();
		REQUIRE(GetSpaceStructureBerth(*source, *core).has_value()); source->name = "Heavy 07";
		this->Supply(SPACE_HEAVY_MINER_MODEL); return *source;
	}
	/** @return Original Assembly footprint world tile consumed by ordinary purchase/clone admission. */
	TileIndex Tile() const { return TileXYL(assembly->state.x, assembly->state.y, MapLayer::Space); }
};

/** Independent physical bills, source identity and native clone/replacement policy remain authoritative for the newly appended role. */
TEST_CASE("Heavy ordinary clone and renewal preserve original paid boundaries", "[heavy-commissioning]")
{
	HeavyCommissioningRig rig; auto &source = rig.Source(); const auto original = GetSpaceCraftReference(source); const auto recipe = GetSpaceVehicleConstruction(SPACE_HEAVY_MINER_MODEL);
	const auto generation = GetNextSpaceVehicleGeneration(); const auto money = Company::Get(source.owner)->money; const auto digest = ComputeSpaceStructureDigest();
	CHECK(PurchaseSpaceVehicleAtFacility({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, rig.Tile(), source.engine_type, INVALID_CARGO).cost.Succeeded());
	CHECK(ComputeSpaceStructureDigest() == digest); CHECK(Company::Get(source.owner)->money == money); CHECK(GetNextSpaceVehicleGeneration() == generation);
	SECTION("Foreign actor and stale factory generation refuse before another paid intent") {
		CHECK(CmdQueueOrbitalManufacture(DoCommandFlag::Execute, rig.assembly->index, rig.assembly->generation + 1, rig.core->index, rig.core->generation, SPACE_HEAVY_MINER_MODEL).Failed());
		{
			AutoRestoreBackup actor(_current_company, CompanyID{1});
			CHECK(CloneSpaceVehicleAtFacility(DoCommandFlag::Execute, rig.Tile(), source, false).cost.Failed());
			CHECK(CmdQueueOrbitalManufacture(DoCommandFlag::Execute, rig.assembly->index, rig.assembly->generation, rig.core->index, rig.core->generation, SPACE_HEAVY_MINER_MODEL).Failed());
		}
		CHECK(ComputeSpaceStructureDigest() == digest); CHECK(GetNextSpaceVehicleGeneration() == generation); CHECK(Company::Get(source.owner)->money == money);
		CHECK(ResolveSpaceCraft(original) == &source); CHECK(GetSpaceVehicleCount() == 1);
	}
	SECTION("Clone consumes a separate complete bill and retains original allowance") {
		source.manifest->returns_remaining = 1;
		// Declared isolated native ore/feeder-share ownership proves cloning cannot duplicate an existing hold; the loaded-world commissioning observer grants no such packets.
		REQUIRE(CargoPacket::CanAllocateItem()); auto *ore = CargoPacket::Create(uint16_t{4}, uint16_t{7}, StationID::Invalid(), source.tile, Money{11});
		const auto packet = ore->index; source.cargo.Append(ore);
		CHECK(CloneSpaceVehicleAtFacility(DoCommandFlag::Execute, rig.Tile(), source, true).cost.Failed()); CHECK(ComputeSpaceStructureDigest() == digest);
		auto [cost, id] = CmdCloneVehicle(DoCommandFlag::Execute, rig.Tile(), source.index, false); REQUIRE(cost.Succeeded()); CHECK(id == VehicleID::Invalid());
		auto &job = *rig.assembly->module->manufacture; REQUIRE(job.clone.has_value()); CHECK(job.clone->source == original); CHECK(GetSpaceVehicleCount() == 1);
		rig.Tick(); rig.Work(recipe->work_ticks); REQUIRE(job.phase == OrbitalManufacturePhase::Complete); const auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
		CHECK(product->generation != original.generation); CHECK(product->name == "Heavy 08"); CHECK(product->cargo_cap == 40); CHECK(product->unitnumber != source.unitnumber);
		CHECK(source.cargo.TotalCount() == 4); CHECK(CargoPacket::Get(packet)->GetFeederShare() == Money{11}); CHECK(product->cargo.TotalCount() == 0);
		CHECK(source.manifest->returns_remaining == 1); CHECK(product->manifest->returns_remaining == 2); CHECK(product->manifest->pods.empty());
		CHECK(ResolveSpaceCraft(original) == &source); CHECK(GetSpaceVehicleCount() == 2); CHECK(GetNextSpaceVehicleGeneration() == generation + 1);
	}
	SECTION("Renewal preserves the original source through cancellation or settlement") {
		REQUIRE(CmdSetAutoReplace(DoCommandFlag::Execute, source.group_id, source.engine_type, source.engine_type, false).Succeeded());
		source.age = source.max_age + 366; // Explicit native age-policy unit input; no simulated calendar or economy claim.
		source.manifest->returns_remaining = 1; const auto unit = source.unitnumber; const auto sale = source.value;
		REQUIRE(CmdAutoreplaceVehicle(DoCommandFlag::Execute, source.index).Succeeded()); auto &job = *rig.assembly->module->manufacture;
		REQUIRE(job.replacement.has_value()); CHECK(job.replacement->source == original); CHECK(GetSpaceVehicleCount() == 1);
		CHECK(CmdAutoreplaceVehicle(DoCommandFlag::Execute, source.index).Failed());
		SECTION("Cancellation returns only this new full bill") {
			rig.Tick(); rig.Work(7); REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, rig.assembly->index, rig.assembly->generation, rig.core->index, rig.core->generation).Succeeded());
			CHECK(ResolveSpaceCraft(original) == &source); CHECK(source.manifest->returns_remaining == 1); CHECK(GetNextSpaceVehicleGeneration() == generation);
			// Cancelling the second producer cannot dismantle the source's original embodied heavy bill.
			for (const auto &input : recipe->inputs) CHECK(rig.core->module->stock[GetCargoTypeByLabel(input.label)].TotalCount() == input.quantity);
		}
		SECTION("Final safe handover preserves the old used allowance and unit") {
			rig.Tick(); rig.Work(recipe->work_ticks - 1); CHECK(ResolveSpaceCraft(original) == &source); CHECK(job.product == SpaceCraftRef{});
			rig.Work(1); REQUIRE(job.phase == OrbitalManufacturePhase::Complete); const auto *product = ResolveSpaceCraft(job.product); REQUIRE(product != nullptr);
			CHECK(ResolveSpaceCraft(original) == nullptr); CHECK(product->unitnumber == unit); CHECK(product->manifest->returns_remaining == 1);
			CHECK(product->generation == generation); CHECK(GetNextSpaceVehicleGeneration() == generation + 1);
			CHECK(product->name == "Heavy 07"); CHECK(product->cargo_cap == 40); CHECK(GetSpaceVehicleCount() == 1);
			CHECK(job.replacement->settled); CHECK(job.replacement->sale_credit == static_cast<uint32_t>(sale));
			const auto settled = ComputeSpaceStructureDigest(); rig.Tick(); rig.Tick(); CHECK(ComputeSpaceStructureDigest() == settled);
		}
	}
}
