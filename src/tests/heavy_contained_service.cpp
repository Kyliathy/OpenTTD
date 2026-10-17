/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_contained_service.cpp Immutable contained bills, actual Core work and isolated shared Dock packet/berth bounds.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CCC
 * Frames, recipe inputs and exhausted quota are explicit unit inputs. Actual paid Core/legacy and newly built Dock acceptance belong to separate native producers.
 */
#include "../stdafx.h"
#include "manufacturing_fixture.h"
#include "../orbital_service.h"
#include "../space_mining_tuning.h"
#include "../space_vehicle_cmd.h"
#include "../openttd.h"
#include "../safeguards.h"

/** @param kind Explicit isolated completed module role. @param id Unit identity. @param x Local left edge. @return Exact consumed detached module, without native packet, power or gameplay rights. */
static SpaceStructureRecord ContainedServiceRecord(SpaceBuildKind kind, uint16_t id, uint16_t x)
{
	const auto &definition = *GetSpaceConstructionDefinition(kind); SpaceStructureRecord row;
	row.ref = {SpaceStructureID{id}, uint64_t{id} + 1}; row.state.kind = kind; row.state.owner = Owner{0}; row.state.x = x; row.state.y = 4;
	row.state.capital_paid = definition.capital; row.state.phase = SpaceStructurePhase::FrameComplete; row.work_ticks = definition.work_ticks;
	row.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)}; row.module.emplace();
	if (kind == SpaceBuildKind::Core) row.module->producer = UtilityProducerID{0};
	else row.module->utility_source = {SpaceStructureID{0}, 1};
	row.module->consumer = UtilityConsumerID{id}; return row;
}

/** @param job Detached sole job owner. @param model Immutable original role or genuine unknown. @param vehicle Distinct unit craft identity. @param first_packet Distinct first input ID; second input uses the next ID. */
static void FillContainedService(OrbitalServiceRecord &job, uint16_t model, uint32_t vehicle, uint32_t first_packet)
{
	job.model = model; job.craft = {VehicleID{vehicle}, uint64_t{vehicle} + 1}; const auto tuning = GetOrbitalServiceJobTuning(job); REQUIRE(tuning.has_value()); job.remaining_ticks = tuning->ticks;
	// Each declared quantity owns its own reference set; one real two-unit packet remains valid for the heavy's complete input bill.
	for (uint i = 0; i < 2; ++i) { job.quantities[i] = tuning->inputs[i]; job.inputs[i] = {{CargoPacketID{first_packet + i}, StationID::Invalid()}}; }
}

/** Actual isolated factory product travels by native orders to its original Core, then uses the unchanged common allocation/settlement owner. */
class HeavyContainedServiceRig : public ManufacturingMaterializationRig {
	AutoRestoreBackup<uint8_t> ore_weight{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"NORE"}))->weight, uint8_t{16}}; ///< Declared one-tonne mineral dimension in this isolated native content world.
public:
	SpaceVehicle *craft = nullptr; ///< Sole actual isolated factory product; unit allowance is declared separately from real gameplay procurement.
	/** @param model Original15 or appended21 service role. @param allowance Explicit starting unit allowance, exhausted by default. */
	explicit HeavyContainedServiceRig(uint16_t model = 21, uint8_t allowance = 0)
	{
		auto &job = this->Start(model); this->Work(GetSpaceVehicleConstruction(model)->work_ticks); REQUIRE(job.phase == OrbitalManufacturePhase::Complete);
		craft = ResolveSpaceCraft(job.product); REQUIRE(craft != nullptr); REQUIRE(craft->manifest != nullptr);
		REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, assembly->index, assembly->generation, core->index, core->generation).Succeeded());
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Insert, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Structure, core->index.base(), core->generation).Succeeded());
		// Native travel/approach arbitration must earn this unit hull's real Core slot; the fixture assigns no pose or reservation to bypass heavy service admission.
		for (uint elapsed = 0; !GetSpaceStructureBerth(*craft, *core); ++elapsed) {
			REQUIRE(elapsed < 3000); AutoRestoreBackup pause(_pause_mode, PauseModes{}); this->Allocate(); PrepareSpaceApproachTick(); craft->Tick();
		}
		craft->manifest->returns_remaining = allowance;
		const auto tuning = *GetSpaceServiceTuning(model); this->Add(*core, CargoLabel{"RFMT"}, tuning.inputs[0]); this->Add(*core, CargoLabel{"ELEC"}, tuning.inputs[1]);
	}
	/** Return any retained isolated service packets through their original command before the parent fixture disposes sole stock owners. */
	~HeavyContainedServiceRig()
	{
		const auto &job = core->module->service;
		if (job.remaining_ticks != 0) { const auto result = CmdServiceOrbitalProbe(DoCommandFlag::Execute, core->index, core->generation, job.craft.vehicle, job.craft.generation, OrbitalServiceControl::Cancel); assert(result.Succeeded()); (void)result; }
	}
	/** @param control Actual contained operation. @param flags Execution or successful mixed query. @return Original Core command outcome. */
	CommandCost Control(OrbitalServiceControl control = OrbitalServiceControl::Start, DoCommandFlags flags = DoCommandFlag::Execute)
	{
		const auto ref = core->module->service.remaining_ticks == 0 ? GetSpaceCraftReference(*craft) : core->module->service.craft;
		return CmdServiceOrbitalProbe(flags, core->index, core->generation, ref.vehicle, ref.generation, control);
	}
	/** @param ticks Exact native opportunities; common allocation and its original participant alone advance work and consume energy. */
	void ServiceWork(uint ticks)
	{
		// Every requested opportunity gathers the same actual job demand before its single shared allocation/settlement; there is no private utility account or countdown.
		for (uint tick = 0; tick < ticks; ++tick) { AutoRestoreBackup pause(_pause_mode, PauseModes{}); this->Allocate(); TickOrbitalServices(); }
	}
};

/** Unknown genuine receipts stay one-part/148ticks; known heavy bills bind two actual units and remain independent across both contained Dock owners. */
TEST_CASE("Contained service immutable roles preserve legacy bills and bound both Dock jobs", "[heavy-contained-service]")
{
	OrbitalServiceState old; const auto original = GetOrbitalServiceJobTuning(old); REQUIRE(original); CHECK(original->ticks == 148); CHECK(original->inputs == std::array<uint8_t, 2>{1, 1});
	old.model = 21; CHECK(GetOrbitalServiceJobTuning(old)->ticks == 296); old.model = 16; CHECK_FALSE(GetOrbitalServiceJobTuning(old));
	auto core = ContainedServiceRecord(SpaceBuildKind::Core, 0, 4); FillContainedService(core.module->service, 21, 1, 1);
	CHECK(ValidateSpaceStructureRecords(std::array{core}, 2, 64, 64)); auto bad = core; bad.module->service.remaining_ticks = 297; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 2, 64, 64));
	bad = core; bad.module->service.model = UINT16_MAX; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 2, 64, 64));
	bad = core; bad.module->service.quantities[0] = 1; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{bad}, 2, 64, 64));
	bad = core; bad.module->service.inputs[0].push_back({CargoPacketID{3}, StationID::Invalid()}); CHECK(ValidateSpaceStructureRecords(std::array{bad}, 2, 64, 64));
	core.module->service = {}; auto dock = ContainedServiceRecord(SpaceBuildKind::Dock, 1, 2); FillContainedService(dock.module->service, 21, 1, 1); FillContainedService(dock.module->dock_service, 21, 2, 3);
	CHECK(ValidateSpaceStructureRecords(std::array{core, dock}, 3, 64, 64)); bad = dock; bad.module->dock_service.craft = bad.module->service.craft; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{core, bad}, 3, 64, 64));
	bad = dock; bad.module->dock_service.inputs[0] = bad.module->service.inputs[0]; CHECK_FALSE(ValidateSpaceStructureRecords(std::array{core, bad}, 3, 64, 64));
}

/** Actual native Core work preserves complete inputs through query/pause/outage, spends750J once per opportunity and restores allowance only at final completion for both old and heavy roles. */
TEST_CASE("Contained Core service earns its immutable original complete bill once", "[heavy-contained-service]")
{
	const auto model = GENERATE(uint16_t{15}, uint16_t{21}); HeavyContainedServiceRig rig(model); auto &job = rig.core->module->service; const auto tuning = *GetSpaceServiceTuning(model);
	const auto structures = ComputeSpaceStructureDigest(), power = ComputeUtilityPowerDigest(); CHECK(rig.Control(OrbitalServiceControl::Start, {DoCommandFlag::Execute, DoCommandFlag::QueryCost}).Succeeded());
	CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == power); REQUIRE(rig.Control().Succeeded()); CHECK(job.model == model); CHECK(job.remaining_ticks == tuning.ticks);
	const auto used = GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j; rig.ServiceWork(37); CHECK(job.remaining_ticks == tuning.ticks - 37); CHECK(GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j == used + 27750);
	REQUIRE(rig.Control(OrbitalServiceControl::Pause).Succeeded()); rig.ServiceWork(9); CHECK(job.remaining_ticks == tuning.ticks - 37); REQUIRE(rig.Control(OrbitalServiceControl::Resume).Succeeded());
	REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, rig.core->index, rig.core->generation, true).Succeeded()); rig.ServiceWork(9); CHECK(job.remaining_ticks == tuning.ticks - 37);
	REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, rig.core->index, rig.core->generation, false).Succeeded()); rig.ServiceWork(job.remaining_ticks - 1);
	CHECK(job.remaining_ticks == 1); CHECK(job.inputs[0].TotalCount() == tuning.inputs[0]); CHECK(job.inputs[1].TotalCount() == tuning.inputs[1]); CHECK(rig.craft->manifest->returns_remaining == 0);
	rig.ServiceWork(1); CHECK(job.remaining_ticks == 0); CHECK(job.model == UINT16_MAX); CHECK(rig.craft->manifest->services_completed == 1); CHECK(rig.craft->manifest->returns_remaining == 2);
	CHECK(GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j == used + uint64_t{tuning.ticks} * 750); rig.ServiceWork(10); CHECK(rig.craft->manifest->services_completed == 1);
}

/** Premature heavy renewal cannot reserve parts; sender loss preserves the immutable complete bill and already spent energy for explicit cancellation. */
TEST_CASE("Contained heavy service refuses top-up and retains its original lost sender bill", "[heavy-contained-service]")
{
	{
		HeavyContainedServiceRig rig(21, 1); const auto structures = ComputeSpaceStructureDigest(); CHECK(rig.Control().Failed()); CHECK(ComputeSpaceStructureDigest() == structures);
	}
	{
		HeavyContainedServiceRig rig; REQUIRE(rig.Control().Succeeded()); rig.ServiceWork(37); auto &job = rig.core->module->service; const auto used = GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j;
		// Explicit isolated native hull destruction exercises retained job ownership; it creates no gameplay casualty, packet compensation or restored allowance.
		delete rig.craft; rig.craft = nullptr; rig.ServiceWork(9); CHECK(job.model == 21); CHECK(job.remaining_ticks == 259); CHECK(job.inputs[0].TotalCount() == 2); CHECK(job.inputs[1].TotalCount() == 2);
		REQUIRE(rig.Control(OrbitalServiceControl::Cancel).Succeeded()); CHECK(job.remaining_ticks == 0); CHECK(rig.core->module->stock[GetSpaceSupplyCargo(SpaceSupply::RefinedMetal)].TotalCount() == 2);
		CHECK(GetUtilityPowerProducer(rig.core->module->producer)->consumed_lifetime_j == used);
	}
}
