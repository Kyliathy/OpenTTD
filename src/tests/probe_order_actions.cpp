/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file probe_order_actions.cpp Whole freight, genuine paid service and typed command contracts for probe orders.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B
 * Geometry, initial packets and exhausted allowance are declared isolated unit inputs. Loaded-world acceptance supplies gameplay travel/service evidence separately.
 */
#include "../stdafx.h"
#include "manufacturing_fixture.h"
#include "../space_order_actions.h"
#include "../space_manifest.h"
#include "../space_structure_cmd.h"
#include "../space_vehicle_cmd.h"
#include "../orbital_service.h"
#include "../safeguards.h"

/** Actual isolated paid hull and original Core handlers; explicit reached geometry does not claim gameplay navigation or procurement. */
class ProbeOrderActionRig : public ManufacturingMaterializationRig {
public:
	/** Materialize the original real recipe, admit its existing Core approach and declare its reached unit position.
	 * @param model Canonical paid role whose freight/service capability is under test.
	 * @param action Actual typed instruction to execute at the existing original Core.
	 * @return Same native paid product, with no duplicate identity or free service history.
	 */
	SpaceVehicle &AtCore(uint16_t model, SpaceOrderAction action)
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{});
		auto &job = this->Start(model); this->Work(GetSpaceVehicleConstruction(model)->work_ticks);
		REQUIRE(job.phase == OrbitalManufacturePhase::Complete);
		auto *craft = ResolveSpaceCraft(job.product); REQUIRE(craft != nullptr);
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert, 0, action,
			SpaceOrderTargetKind::Structure, core->index.base(), core->generation).Succeeded());
		const SpaceOrderTarget target{SpaceOrderTargetKind::Structure, core->index.base(), core->generation};
		REQUIRE(RequestSpaceApproach(*craft, target, true)); PrepareSpaceApproachTick();
		REQUIRE(craft->space_orders.approach.has_value()); REQUIRE(craft->space_orders.approach->slot != UINT8_MAX);
		craft->position = *GetSpaceStructureApproachSlot(*core, craft->space_orders.approach->slot); craft->UpdateSpacePosition();
		REQUIRE(GetSpaceStructureBerth(*craft, *core).has_value());
		return *craft;
	}
};

/** Destination congestion retains every original freight packet until the existing module exchange can admit the entire actual hold. */
TEST_CASE("Typed orbital Unload waits for whole destination capacity then transfers once", "[probe-order-actions]")
{
	ProbeOrderActionRig rig;
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); // The isolated world's effects pause cannot suppress the actual reached-order/approach participant being exercised.
	auto &craft = rig.AtCore(20, SpaceOrderAction::Unload);
	const auto cargo = GetCargoTypeByLabel(CargoLabel{"GOOD"});
	rig.Add(*rig.core, CargoLabel{"GOOD"}, 1023);
	REQUIRE(CargoPacket::CanAllocateItem());
	craft.cargo_type = cargo; craft.cargo_cap = GetSpaceFreightCapacity(20, cargo);
	auto *packet = CargoPacket::Create(uint16_t{2}, uint16_t{7}, StationID::Invalid(), craft.tile, Money{37});
	packet->UpdateLoadingTile(craft.tile); // Declared unit freight must still enter native vehicle custody before a real unloading command can release it.
	craft.cargo.Append(packet);
	const auto money = Company::Get(craft.owner)->money, value = craft.value;
	const auto generation = GetNextSpaceVehicleGeneration();
	REQUIRE(TickSpaceOrders(craft));
	CHECK(craft.space_orders.cursor == 0); CHECK(craft.space_orders.started); CHECK(craft.space_orders.timer_ticks == 1);
	CHECK(craft.space_orders.blocked == SpaceOrderBlock::ProducerUnavailable);
	CHECK(craft.cargo.TotalCount() == 2); CHECK(rig.core->module->stock[cargo].TotalCount() == 1023);
	CHECK(ValidateNativeSpaceOrders());
	REQUIRE(HubCargoTransfer::Consume(rig.core->module->stock[cargo], 1));
	REQUIRE(TickSpaceOrders(craft));
	CHECK(craft.cargo.TotalCount() == 0); CHECK(rig.core->module->stock[cargo].TotalCount() == 1024);
	CHECK(craft.space_orders.cursor == 1); CHECK(craft.space_orders.blocked == SpaceOrderBlock::Completed);
	REQUIRE(TickSpaceOrders(craft));
	CHECK(rig.core->module->stock[cargo].TotalCount() == 1024); CHECK(Company::Get(craft.owner)->money == money);
	CHECK(craft.value == value); CHECK(GetNextSpaceVehicleGeneration() == generation);
}

/** An explicit reached service action uses actual RFMT/ELEC and original common energy; its queue neither counts work nor grants a second allowance. */
TEST_CASE("Typed Return Service retains one original paid Core service job", "[probe-order-actions]")
{
	ProbeOrderActionRig rig;
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); // Common electrical work must run after the isolated map setup's graphics/effects pause has been lifted.
	auto &craft = rig.AtCore(19, SpaceOrderAction::ReturnService);
	REQUIRE(craft.manifest != nullptr); auto &manifest = *craft.manifest;
	manifest.returns_remaining = 0; // Declared isolated exhausted-service input; loaded-world tests must exhaust actual paid returns.
	REQUIRE(TickSpaceOrders(craft));
	CHECK(craft.space_orders.blocked == SpaceOrderBlock::ServiceRequired); CHECK_FALSE(HasOrbitalServiceAssignment(craft));
	CHECK(ValidateNativeSpaceOrders());
	rig.Add(*rig.core, CargoLabel{"RFMT"}, 1); rig.Add(*rig.core, CargoLabel{"ELEC"}, 1);
	REQUIRE(TickSpaceOrders(craft)); REQUIRE(HasOrbitalServiceAssignment(craft));
	CHECK(rig.core->module->service.remaining_ticks == GetMultiLayerSpaceBudget().service_ticks);
	CHECK(manifest.service_ticks == 0); CHECK(manifest.services_completed == 0); CHECK(manifest.returns_remaining == 0);
	CHECK(craft.space_orders.work_ticks == 0); CHECK(craft.space_orders.cursor == 0);
	const auto consumed = GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j;
	// Every real shared opportunity advances the contained original job; a repeated callback in that same epoch must never count twice.
	for (uint step = 0; step < GetMultiLayerSpaceBudget().service_ticks; ++step) {
		rig.Allocate(); TickOrbitalServices(); const auto remaining = rig.core->module->service.remaining_ticks;
		TickOrbitalServices(); CHECK(rig.core->module->service.remaining_ticks == remaining);
		CHECK(craft.space_orders.cursor == 0); CHECK(craft.space_orders.work_ticks == 0);
	}
	CHECK(manifest.services_completed == 1); CHECK(manifest.returns_remaining == GetMultiLayerSpaceBudget().returns_per_service);
	CHECK(GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j == consumed +
		uint64_t{GetMultiLayerSpaceBudget().service_ticks} * GetMultiLayerSpaceBudget().work_j_per_tick);
	CHECK_FALSE(HasOrbitalServiceAssignment(craft)); REQUIRE(TickSpaceOrders(craft)); CHECK(craft.space_orders.cursor == 1);
	REQUIRE(TickSpaceOrders(craft)); CHECK(manifest.services_completed == 1);
	CHECK(rig.core->module->stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount() == 0);
	CHECK(rig.core->module->stock[GetSpaceSupplyCargo(SpaceSupply::Electronics)].TotalCount() == 0);
}

/** Quoting a valid edit, including mixed Execute/QueryCost, cannot restart native work or release a physical berth; incompatible roles refuse with every owner intact. */
TEST_CASE("Typed probe order quotes preserve original physical owners and role refusals", "[probe-order-actions]")
{
	ProbeOrderActionRig rig; auto &craft = rig.AtCore(20, SpaceOrderAction::Visit);
	const auto orders = ComputeSpaceOrdersDigest(), structures = ComputeSpaceStructureDigest(), manifests = ComputeSpaceManifestDigest();
	const auto money = Company::Get(craft.owner)->money;
	const auto random = _random;
	// Both supported estimates and role-invalid execution must preserve the same actual queue, berth, packets, capital and simulation RNG.
	for (const auto flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) {
		CHECK(CmdEditSpaceOrder(flags, craft.index, craft.generation, SpaceOrderEdit::Insert, 1, SpaceOrderAction::Visit,
			SpaceOrderTargetKind::Structure, rig.core->index.base(), rig.core->generation).Succeeded());
	}
	CHECK(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation, SpaceOrderEdit::Insert, 1, SpaceOrderAction::Survey,
		SpaceOrderTargetKind::Body, 0, 0).Failed());
	CHECK(CmdEditSpaceOrder(DoCommandFlag::Execute, craft.index, craft.generation + 1, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit,
		SpaceOrderTargetKind::Body, 0, 0).Failed());
	CHECK(ComputeSpaceOrdersDigest() == orders); CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeSpaceManifestDigest() == manifests);
	CHECK(Company::Get(craft.owner)->money == money); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** The existing network/replay argument codec retains full typed target width and exact generations; no ordinary StationID destination participates. */
TEST_CASE("Typed probe instructions retain full body IDs and reached service waits", "[probe-order-actions]")
{
	using Args = CommandTraits<Commands::EditSpaceOrder>::Args;
	const Args args{VehicleID{70000}, uint64_t{0x123456789abcdefULL}, SpaceOrderEdit::Insert, uint8_t{0},
		SpaceOrderAction::Mine, SpaceOrderTargetKind::Body, uint32_t{90000}, uint64_t{0}};
	const auto buffer = EndianBufferWriter<CommandDataBuffer>::FromValue(args);
	CHECK(buffer.size() == 28); CHECK(EndianBufferReader::ToValue<Args>(buffer) == args);
	SpaceOrderRecord row{VehicleID{1}, 2, {}};
	row.queue.entries = {{SpaceOrderAction::ReturnService, {SpaceOrderTargetKind::LaunchCenter, 1, 7}}};
	row.queue.started = true; row.queue.timer_ticks = 1; row.queue.blocked = SpaceOrderBlock::ServiceRequired;
	CHECK(ValidateSpaceOrderRecords(std::array{row}, 3)); CHECK(IsSpaceReturnServiceWait(row.queue));
	row.queue.blocked = SpaceOrderBlock::ReturnsPending; CHECK(ValidateSpaceOrderRecords(std::array{row}, 3));
	row.queue.started = false; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{row}, 3)); CHECK_FALSE(IsSpaceReturnServiceWait(row.queue));
	row.queue.started = true; row.queue.timer_ticks = 0; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{row}, 3));
	row.queue.timer_ticks = 1; row.queue.work_ticks = 1; CHECK_FALSE(ValidateSpaceOrderRecords(std::array{row}, 3));
}
