/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_reservation.cpp Isolated actual packet/grant admission and malformed copied-owner boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group7
 * Detached paid frames and explicit unit packets characterize contracts only; native Surface freight/payment proof has a separate observer.
 */
#include "../stdafx.h"
#include "mock_environment.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../orbital_manufacturing.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../command_func.h"
#include "../space_approach.h"
#include "../space_vehicle.h"
#include "../hub_cargo.h"
#include "../company_base.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../transport_tube.h"
#include "../timer/timer_game_tick.h"
#include "excavation_fixture.h"
#include "../safeguards.h"

#include "manufacturing_fixture.h"

/** Typed requests preserve all five roles, pure estimates and paid issuance without acquiring a premature hull or packet batch. */
TEST_CASE("Manufacturing requests retain complete recipes without immediate materialization", "[manufacturing-reservation]")
{
	// Each role uses a fresh explicit contract world, keeping its packet/serial/funds proof independent of another model's recipe.
	for (const auto model : std::array<uint16_t, 5>{15, 19, 20, 16, 17}) {
		CAPTURE(model); ManufacturingReservationRig rig; rig.Supply(model);
		const auto original = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(); const auto random = _random;
		const auto cash = Company::Get(_current_company)->money; const auto hulls = GetSpaceVehicleCount();
		REQUIRE(rig.Queue(model).Succeeded()); CHECK(ComputeSpaceStructureDigest() == original); CHECK(ComputeUtilityPowerDigest() == utility);
		CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
		REQUIRE(rig.Queue(model, DoCommandFlag::Execute).Succeeded());
		const auto &job = *rig.assembly->module->manufacture; CHECK(job.phase == OrbitalManufacturePhase::Queued); CHECK(job.serial == 1);
		// Direct command contracts do not simulate framework payment; the native observer must prove the actual once-only debit.
		CHECK(Company::Get(_current_company)->money == cash); CHECK(GetSpaceVehicleCount() == hulls);
		for (const auto &slot : job.inputs) CHECK(slot.TotalCount() == 0);
		CHECK(rig.Queue(model).Failed()); TickOrbitalManufacturingReservations(); CHECK(job.phase == OrbitalManufacturePhase::Queued);
		REQUIRE(ValidateSpaceStructures());
		REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, rig.assembly->index, rig.assembly->generation, SpaceStructureRef{}.structure, 0).Succeeded());
		CHECK(rig.assembly->module->manufacture == nullptr); CHECK(rig.assembly->module->manufacture_serial == 1);
	}
}

/** A real common allocation gates the whole four-input batch, while outage/retry/cancellation preserve actual material and energy ownership. */
TEST_CASE("Manufacturing reserves every heavy input once after a real shared grant", "[manufacturing-reservation]")
{
	ManufacturingReservationRig rig; rig.Supply(16, 1); REQUIRE(rig.Queue(16, DoCommandFlag::Execute).Succeeded());
	REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, rig.core->index, rig.core->generation, true).Succeeded()); rig.Opportunity();
	CHECK(rig.assembly->module->manufacture->phase == OrbitalManufacturePhase::Queued);
	REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, rig.core->index, rig.core->generation, false).Succeeded()); rig.Opportunity();
	const auto &job = *rig.assembly->module->manufacture; REQUIRE(job.phase == OrbitalManufacturePhase::Reserved);
	CHECK(job.reservation_interval == GetUtilityPowerInterval()); CHECK(GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j == 0);
	// All four split quantities preserve physical age/provenance and the combined feeder share; no early common label moves alone.
	const auto &recipe = *GetSpaceVehicleConstruction(16);
	for (size_t input = 0; input < recipe.inputs.size(); ++input) {
		const auto cargo = GetCargoTypeByLabel(recipe.inputs[input].label);
		CHECK(job.inputs[input].TotalCount() == recipe.inputs[input].quantity); CHECK(rig.core->module->stock[cargo].TotalCount() == 1);
		const auto *packet = job.inputs[input].Packets()->begin()->second.front();
		CHECK(packet->GetPeriodsInTransit() == 7); CHECK(packet->GetSourceTile() == TileXYL(2, 2, MapLayer::Surface));
		CHECK(packet->GetFeederShare() == Money{recipe.inputs[input].quantity * 10});
	}
	const auto reserved = ComputeSpaceStructureDigest(); rig.Opportunity(); CHECK(ComputeSpaceStructureDigest() == reserved);
	CHECK(CmdDisconnectOrbitalServices({}, rig.assembly->index, rig.assembly->generation).Failed());
	CHECK(ClassifyOrbitalModuleRemoval(*rig.assembly, _current_company) == OrbitalRemovalBlock::Construction);
	rig.Add(*rig.storage, CargoLabel{"ELEC"}, 1024); const auto full = ComputeSpaceStructureDigest();
	CHECK(CmdCancelOrbitalManufacture({}, rig.assembly->index, rig.assembly->generation, rig.storage->index, rig.storage->generation).Failed());
	CHECK(ComputeSpaceStructureDigest() == full);
	REQUIRE(HubCargoTransfer::Consume(rig.storage->module->stock[GetCargoTypeByLabel(CargoLabel{"ELEC"})], 1024));
	REQUIRE(CmdEvacuateOrbitalModule(DoCommandFlag::Execute, rig.assembly->index, rig.assembly->generation, rig.storage->index, rig.storage->generation).Succeeded());
	CHECK(rig.assembly->module->manufacture == nullptr); CHECK(rig.assembly->module->manufacture_serial == 1);
	for (const auto &input : recipe.inputs) CHECK(rig.storage->module->stock[GetCargoTypeByLabel(input.label)].TotalCount() == input.quantity);
	CHECK(GetSpaceVehicleCount() == 0); REQUIRE(ValidateSpaceStructures());
}

/** Copied previews cannot invent a role, original source, serial, epoch or partial input batch. */
TEST_CASE("Manufacturing copied owners reject unpaid and aliased reservations", "[manufacturing-reservation]")
{
	ManufacturingReservationRig rig; rig.Supply(15); REQUIRE(rig.Queue(15, DoCommandFlag::Execute).Succeeded()); rig.Opportunity();
	const auto rows = CaptureSpaceStructures(); REQUIRE(ValidateSpaceStructureRecords(rows, 4, 64, 64));
	auto bad = rows; ++bad[1].module->manufacture->source.generation; CHECK_FALSE(ValidateSpaceStructureRecords(bad, 4, 64, 64));
	bad = rows; bad[1].module->manufacture->model = 14; CHECK_FALSE(ValidateSpaceStructureRecords(bad, 4, 64, 64));
	bad = rows; bad[1].module->manufacture->phase = OrbitalManufacturePhase::Queued; CHECK_FALSE(ValidateSpaceStructureRecords(bad, 4, 64, 64));
	bad = rows; --bad[1].module->manufacture->quantities[1]; CHECK_FALSE(ValidateSpaceStructureRecords(bad, 4, 64, 64));
	bad = rows; bad[1].module->manufacture->serial = 0; CHECK_FALSE(ValidateSpaceStructureRecords(bad, 4, 64, 64));
	bad = rows; bad[1].module->manufacture->reservation_interval = GetUtilityPowerInterval() + 1; CHECK_FALSE(ValidateOrbitalManufacturingRecords(bad, GetUtilityPowerInterval()));
	bad = rows; bad[1].module->manufacture->inputs[1] = bad[1].module->manufacture->inputs[0]; CHECK_FALSE(ValidateSpaceStructureRecords(bad, 4, 64, 64));
	CHECK(GetSpaceVehicleCount() == 0); REQUIRE(ValidateSpaceStructures());
}

/** Missing late labels, funds, original stock incarnations and a sub-quantum actual grant all preserve the entire batch. */
TEST_CASE("Manufacturing admission preserves stock for missing inputs funds or grant", "[manufacturing-reservation]")
{
	ManufacturingReservationRig rig; rig.Supply(16);
	const auto electronics = GetCargoTypeByLabel(CargoLabel{"ELEC"}); REQUIRE(HubCargoTransfer::Consume(rig.core->module->stock[electronics], 2));
	const auto incomplete = ComputeSpaceStructureDigest(); CHECK(rig.Queue(16).Failed()); CHECK(ComputeSpaceStructureDigest() == incomplete);
	rig.Add(*rig.core, CargoLabel{"ELEC"}, 2);
	const auto complete = ComputeSpaceStructureDigest();
	{
		AutoRestoreBackup funds(Company::Get(_current_company)->money, Money{0});
		CHECK(rig.Queue(16).Failed()); CHECK(ComputeSpaceStructureDigest() == complete);
	}
	CHECK(CmdQueueOrbitalManufacture({}, rig.assembly->index, rig.assembly->generation, rig.core->index, rig.core->generation + 1, 16).Failed());
	CHECK(ComputeSpaceStructureDigest() == complete); CHECK(rig.assembly->module->manufacture == nullptr);
	REQUIRE(rig.Queue(16, DoCommandFlag::Execute).Succeeded());
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); rig.Allocate();
		const auto consumer = rig.assembly->module->consumer;
		const auto grant = GetUtilityAllocation(consumer); REQUIRE(grant.has_value()); REQUIRE(grant->granted_energy_j == 750);
		const auto release = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {consumer, GetUtilityPowerInterval()}, 1);
		REQUIRE(CommitUtilityMutation(release).error == UtilityPowerError::None);
		const auto before = ComputeSpaceStructureDigest();
		// A positive physical panel and complete materials do not replace the actual full grant:749J cannot acquire this whole750J admission batch.
		TickOrbitalManufacturingReservations(); CHECK(ComputeSpaceStructureDigest() == before);
		CHECK(rig.assembly->module->manufacture->phase == OrbitalManufacturePhase::Queued);
		CHECK(GetUtilityAllocation(consumer)->granted_energy_j == 749);
	}
	REQUIRE(ValidateSpaceStructures());
}
