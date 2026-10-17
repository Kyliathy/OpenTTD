/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file processor_freight.cpp Canonical hauler refits and exclusive original-packet exchange contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12
 * Explicit isolated frames and packets establish component behavior; separate native runs own procurement/travel claims.
 */
#include "../stdafx.h"
#include "mock_environment.h"
#include "processor_freight_fixture.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../fios.h"
#include "../saveload/space_cargo_preview.h"
#include "../safeguards.h"

/** Every declared product/refit keeps its physical units and original packet source through a paid factory hauler's exchange. */
TEST_CASE("Paid haulers exchange canonical product packets without rewriting their provenance", "[processor-freight]")
{
	ProcessorFreightRig rig; auto &job = rig.Start(17); rig.Work(GetSpaceVehicleConstruction(17)->work_ticks);
	REQUIRE(job.phase == OrbitalManufacturePhase::Complete); auto *hauler = ResolveSpaceCraft(job.product); REQUIRE(hauler);
	// This declared component layout reaches a real Storage reservation, matching the existing replacement fixture's reached-berth input. Assembly has no general stock; separate paid-world tests prove actual navigation without this isolated layout pose.
	AutoRestoreBackup running(_pause_mode, PauseModes{}); ReleaseSpaceApproach(*hauler);
	const SpaceOrderTarget target{SpaceOrderTargetKind::Structure, rig.storage->index.base(), rig.storage->generation};
	REQUIRE(RequestSpaceApproach(*hauler, target, true)); PrepareSpaceApproachTick(); REQUIRE(hauler->space_orders.approach);
	REQUIRE(hauler->space_orders.approach->slot != UINT8_MAX);
	hauler->position = *GetSpaceStructureApproachSlot(*rig.storage, hauler->space_orders.approach->slot); hauler->UpdateSpacePosition();
	hauler->space_orders.entries = {{SpaceOrderAction::Visit, target}}; hauler->space_orders.cursor = 1; hauler->space_orders.blocked = SpaceOrderBlock::Completed;
	REQUIRE(GetSpaceStructureBerth(*hauler, *rig.storage));
	// Each source bucket has independent actual packets. Switching an empty hold may change its refit; a loaded hold must retain the old cargo, source, age and feeder credit.
	for (const auto label : {CargoLabel{"STEL"}, CargoLabel{"GOOD"}, CargoLabel{"RFMT"}, CargoLabel{"PROP"}, CargoLabel{"NORE"}, CargoLabel{"RARE"}}) {
		CAPTURE(label); const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo)); rig.Add(*rig.storage, label, 3);
		auto &stock = rig.storage->module->stock[cargo]; const auto before = stock.TotalCount(); const auto capacity = GetSpaceFreightCapacity(17, cargo);
		CHECK(capacity == GetMultiLayerSpaceBudget().probe_hold_sixteenths / CargoSpec::Get(cargo)->weight);
		const auto digest = ComputeSpaceStructureDigest(); const auto cash = Company::Get(_current_company)->money;
		REQUIRE(CmdMoveOrbitalFreight({}, rig.storage->index, rig.storage->generation, hauler->index, hauler->generation, cargo, 2, true).Succeeded());
		CHECK(ComputeSpaceStructureDigest() == digest); CHECK(hauler->cargo.TotalCount() == 0);
		REQUIRE(CmdMoveOrbitalFreight(DoCommandFlag::Execute, rig.storage->index, rig.storage->generation, hauler->index, hauler->generation, cargo, 2, true).Succeeded());
		CHECK(stock.TotalCount() == before - 2); CHECK(hauler->cargo.TotalCount() == 2); CHECK(hauler->cargo_type == cargo); CHECK(ValidateSpaceProduction());
		// Actual retained fragments preserve provenance and feeder shares; fitting a different cargo into remaining mass cannot relabel a loaded hold.
		for (const auto *packet : *hauler->cargo.Packets()) { CHECK(packet->GetSource().id == Source::Invalid); CHECK(packet->GetFeederShare() == Money{packet->Count() * 10}); }
		const auto other = GetCargoTypeByLabel(label == CargoLabel{"GOOD"} ? CargoLabel{"STEL"} : CargoLabel{"GOOD"});
		CHECK(CmdMoveOrbitalFreight(DoCommandFlag::Execute, rig.storage->index, rig.storage->generation, hauler->index, hauler->generation, other, 1, true).Failed());
		CHECK(CmdMoveOrbitalFreight(DoCommandFlag::Execute, rig.storage->index, rig.storage->generation + 1, hauler->index, hauler->generation, cargo, 1, false).Failed());
		REQUIRE(CmdMoveOrbitalFreight(DoCommandFlag::Execute, rig.storage->index, rig.storage->generation, hauler->index, hauler->generation, cargo, 2, false).Succeeded());
		CHECK(hauler->cargo.TotalCount() == 0); CHECK(stock.TotalCount() == before); CHECK(Company::Get(_current_company)->money == cash);
	}
}

/** Catalogue labels and inclusive mass bounds cannot grant mining/survey roles general processor freight or haulers undeclared cargo. */
TEST_CASE("Hauler refits retain published units and paid-role packet restrictions", "[processor-freight]")
{
	ProcessorFreightRig rig;
	const auto goods = GetCargoTypeByLabel(CargoLabel{"GOOD"}), steel = GetCargoTypeByLabel(CargoLabel{"STEL"}), passengers = GetCargoTypeByLabel(CargoLabel{"PASS"});
	const auto capabilities = GetMultiLayerVehicleCapabilities();
	// Every original common-product carrier publishes both actual construction labels and the Express form used by Goods crates; label/role metadata must agree before packet movement can exercise a refit.
	for (uint16_t model : {14, 17, 18, 20}) {
		const auto capability = std::ranges::find(capabilities, model, &LayerVehicleCapability::model_id); REQUIRE(capability != capabilities.end());
		CHECK(capability->cargo_classes.Test(CargoClass::Express));
		CHECK(std::ranges::find(capability->cargo_labels, CargoLabel{"STEL"}) != capability->cargo_labels.end());
		CHECK(std::ranges::find(capability->cargo_labels, CargoLabel{"GOOD"}) != capability->cargo_labels.end());
	}
	CHECK(GetSpaceFreightCapacity(17, goods) == 40); CHECK(GetSpaceFreightCapacity(17, steel) == 20);
	CHECK(GetSpaceFreightCapacity(14, goods) == 60); CHECK(GetSpaceFreightCapacity(14, steel) == 30);
	CHECK(GetSpaceFreightCapacity(17, passengers) == 0); CHECK_FALSE(CanRetainSpaceFreight(17, SpacePlacement::Orbital, passengers, 1));
	CHECK_FALSE(CanRetainSpaceFreight(17, SpacePlacement::Orbital, passengers, 0));
	rig.Add(*rig.core, CargoLabel{"GOOD"}, 1); const auto *packet = *rig.core->module->stock[goods].Packets()->begin()->second.begin();
	SpaceConstructionState paid; paid.content_version = MULTILAYER_SPACE_CONTENT_VERSION; paid.placement = SpacePlacement::Orbital;
	// The same original non-asteroid packet belongs to transport roles only; a role switch cannot turn imported/processed goods into a miner's finite extraction history.
	for (uint16_t model : {15, 19, 20, 17}) { paid.model = model; CHECK(CanRetainSpaceCargoPacket(paid, *packet) == (model == 20 || model == 17)); }
	paid.model = 17; paid.content_version = 0; CHECK_FALSE(CanRetainSpaceCargoPacket(paid, *packet));
}

/** Detached hauler admission uses arbitrary same-file ordinals and real packet quantities rather than the open world's CargoSpec table. */
TEST_CASE("Copied hauler dimensions join canonical meanings and complete physical quantities", "[processor-freight]")
{
	LoadCheckData data; data.gameplay_profile_id = to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry); data.native_cargo_dimensions_present = true;
	data.native_cargo_dimension_rows.push_back({CargoType{42}, CargoLabel{"GOOD"}, 8});
	auto &product = data.space_production_rows.emplace_back(); product.craft = {VehicleID{3}, 7}; product.state.model = 17;
	auto &hull = data.native_space_vehicle_rows.emplace_back(); hull.vehicle = product.craft.vehicle; hull.freight_cargo = CargoType{42}; hull.freight_capacity = 40;
	hull.cargo_packet_refs = {6}; data.native_cargo_packet_rows.push_back({CargoPacketID{5}, 2});
	REQUIRE(ValidateSpaceCargoPreviewDimensions(data));
	// The candidate's original cargo meaning controls admission even when a different active-world cargo occupies ordinal42; positive weight alone cannot authorize passengers or an oversized hold.
	data.native_cargo_dimension_rows[0].label = CargoLabel{"PASS"}; CHECK_FALSE(ValidateSpaceCargoPreviewDimensions(data)); data.native_cargo_dimension_rows[0].label = CargoLabel{"GOOD"};
	hull.freight_capacity = 41; CHECK_FALSE(ValidateSpaceCargoPreviewDimensions(data)); hull.freight_capacity = 40;
	data.native_cargo_packet_rows[0].quantity = 41; CHECK_FALSE(ValidateSpaceCargoPreviewDimensions(data)); data.native_cargo_packet_rows[0].quantity = 2;
	hull.cargo_packet_refs = {7}; CHECK_FALSE(ValidateSpaceCargoPreviewDimensions(data)); hull.cargo_packet_refs = {6};
	CHECK(ValidateSpaceCargoPreviewDimensions(data));
}
