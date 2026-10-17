/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_onboard_service.cpp Per-model onboard bills, source admission and original native service pause/completion boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CC
 * Factory frames, service parts, ore dimensions and exhausted quota are declared isolated unit inputs; this file grants no saved gameplay/procurement acceptance.
 */
#include "../stdafx.h"
#include "manufacturing_fixture.h"
#include "../space_manifest.h"
#include "../space_vehicle_cmd.h"
#include "../launch_center_cmd.h"
#include "../openttd.h"
#include "../safeguards.h"

/** Isolated actual paid-factory owner with explicitly declared onboard parts/quota, using the original public service command and native manifest work. */
class HeavyOnboardServiceRig : public ManufacturingMaterializationRig {
	AutoRestoreBackup<uint8_t> ore_weight{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"NORE"}))->weight, uint8_t{16}}; ///< Declared one-tonne default mineral dimension for native factory hull/refit admission.
	AutoRestoreBackup<PauseModes> service_pause{_pause_mode, PauseModes{}}; ///< Explicit normal unit opportunities override the base excavation rig's graphics-suppression pause; individual pause tests restore their own real pause state.
public:
	SpaceVehicle *craft = nullptr; ///< Sole original isolated factory product, never an Engine/pod/manifest substitute.
	/** @param model Existing15 or appended21 logical miner. @param allowance Declared unit starting allowance; default exhausted permits actual onboard service. */
	explicit HeavyOnboardServiceRig(uint16_t model = 21, uint8_t allowance = 0)
	{
		auto &job = this->Start(model); this->Work(GetSpaceVehicleConstruction(model)->work_ticks); REQUIRE(job.phase == OrbitalManufacturePhase::Complete);
		craft = ResolveSpaceCraft(job.product); REQUIRE(craft != nullptr); REQUIRE(craft->manifest != nullptr);
		REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, assembly->index, assembly->generation, core->index, core->generation).Succeeded());
		REQUIRE(CmdEditSpaceOrder(DoCommandFlag::Execute, craft->index, craft->generation, SpaceOrderEdit::Clear, 0, SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, 0, 0).Succeeded());
		REQUIRE(craft->mission.phase == SpaceMissionPhase::Holding); auto &manifest = *craft->manifest; manifest.returns_remaining = allowance;
		const auto service = GetSpaceServiceTuning(model); REQUIRE(service.has_value());
		// These explicit isolated inputs exercise whole native part consumption, not a Surface supply producer or a paid-world service claim.
		for (uint i = 0; i < 2; ++i) {
			REQUIRE(CargoPacket::CanAllocateItem()); manifest.supplies[i + 1].Append(CargoPacket::Create(uint16_t{service->inputs[i]}, uint16_t{0}, StationID::Invalid(), craft->tile, Money{0}), StationID::Invalid());
		}
	}
	/** @param flags Actual command flags, including mixed query validation. @param pause Whether to pause the same existing job instead of starting/resuming it. @return Original native admission/control outcome. */
	CommandCost Control(DoCommandFlags flags = DoCommandFlag::Execute, bool pause = false)
	{
		return CmdServiceLaunchProbe(flags, craft->index, craft->generation, pause);
	}
	/** @param ticks Declared normal native opportunities; only the existing original manifest driver changes its countdown/parts. */
	void WorkService(uint ticks)
	{
		// Every declared opportunity invokes the same whole family driver; no fixture assigns earned service progress or completion receipts.
		for (uint tick = 0; tick < ticks; ++tick) TickSpaceManifests();
	}
};

/** Pure service quantities derive the existing immutable role contracts, while source-less/old-version records retain their original admission limits. */
TEST_CASE("Heavy onboard service quantities and saved capability gates remain per model", "[heavy-onboard-service]")
{
	// Each delivered service role keeps its own work and actual part bill; unrelated capacity holders cannot borrow a probe-service producer.
	for (uint16_t model : {uint16_t{15}, uint16_t{19}, uint16_t{20}, uint16_t{21}}) {
		const auto service = GetSpaceServiceTuning(model); REQUIRE(service.has_value()); CHECK(service->ticks == (model == 21 ? 296 : 148));
		CHECK(service->inputs == (model == 21 ? std::array<uint8_t, 2>{2, 2} : std::array<uint8_t, 2>{1, 1})); CHECK(service->returns == 2);
	}
	for (uint16_t model : {uint16_t{0}, uint16_t{14}, uint16_t{16}, uint16_t{17}, uint16_t{18}, uint16_t{22}}) CHECK_FALSE(GetSpaceServiceTuning(model).has_value());
	SpaceManifestRecord row; row.craft = {VehicleID{22}, 8}; row.service_ticks = 296; row.service_location = SpaceServiceLocation::Onboard;
	CHECK_FALSE(ValidateSpaceManifestRecords(std::array{row})); CHECK(ValidateSpaceManifestRecords(std::array{row}, true));
	CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, row, true, true)); CHECK(IsSpaceHeavyResupplyStateSupported(21, row, true, true, true));
	CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, row, true, false, true)); CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, row, false, true, true));
	row.service_ticks = 297; CHECK_FALSE(ValidateSpaceManifestRecords(std::array{row}, true)); CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, row, true, true, true));
	row.service_ticks = 259; row.returns_remaining = 1; CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, row, true, true, true));
	row.returns_remaining = 0; row.service_location = SpaceServiceLocation::EarthWorkshop; CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, row, true, true, true));
	row.service_ticks = 0; row.services_completed = 1; CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, row, true, true)); CHECK(IsSpaceHeavyResupplyStateSupported(21, row, true, true, true));
}

/** Both actual isolated native roles consume their complete original bill only after earned work, retain partial pauses and leave successful mixed queries unchanged. */
TEST_CASE("Original onboard service earns per-model work and consumes the complete bill once", "[heavy-onboard-service]")
{
	const uint16_t model = GENERATE(uint16_t{15}, uint16_t{21}); HeavyOnboardServiceRig rig(model); auto &manifest = *rig.craft->manifest; const auto service = *GetSpaceServiceTuning(model);
	const auto before = ComputeSpaceManifestDigest(); CHECK(rig.Control({DoCommandFlag::Execute, DoCommandFlag::QueryCost}).Succeeded()); CHECK(ComputeSpaceManifestDigest() == before);
	REQUIRE(rig.Control().Succeeded()); CHECK(manifest.service_ticks == service.ticks); CHECK(manifest.returns_remaining == 0); rig.WorkService(37); CHECK(manifest.service_ticks == service.ticks - 37);
	REQUIRE(rig.Control(DoCommandFlag::Execute, true).Succeeded()); rig.WorkService(9); CHECK(manifest.service_ticks == service.ticks - 37); CHECK(manifest.service_paused);
	const auto paused = ComputeSpaceManifestDigest(); CHECK(rig.Control({DoCommandFlag::Execute, DoCommandFlag::QueryCost}).Succeeded()); CHECK(ComputeSpaceManifestDigest() == paused);
	REQUIRE(rig.Control().Succeeded());
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal}); const auto authority = ComputeSpaceManifestDigest(); rig.WorkService(7); CHECK(ComputeSpaceManifestDigest() == authority); CHECK_FALSE(IsSpaceServiceWorkTick(*rig.craft));
	}
	{
		AutoRestoreBackup editor(_game_mode, GameMode::Editor); const auto authority = ComputeSpaceManifestDigest(); rig.WorkService(7); CHECK(ComputeSpaceManifestDigest() == authority); CHECK_FALSE(IsSpaceServiceWorkTick(*rig.craft));
	}
	rig.WorkService(manifest.service_ticks - 1); CHECK(manifest.service_ticks == 1); CHECK(manifest.supplies[1].TotalCount() == service.inputs[0]); CHECK(manifest.supplies[2].TotalCount() == service.inputs[1]);
	CHECK(manifest.services_completed == 0); CHECK(manifest.returns_remaining == 0); rig.WorkService(1);
	CHECK(manifest.service_ticks == 0); CHECK(manifest.supplies[1].TotalCount() == 0); CHECK(manifest.supplies[2].TotalCount() == 0); CHECK(manifest.services_completed == 1); CHECK(manifest.returns_remaining == service.returns);
	CHECK(manifest.service_location == SpaceServiceLocation::EarthWorkshop); rig.WorkService(10); CHECK(manifest.services_completed == 1); CHECK(manifest.returns_remaining == service.returns);
}

/** Sufficient actual parts never authorize a heavy top-up before exhaustion, and one missing part freezes rather than partially spends an already owned service bill. */
TEST_CASE("Heavy onboard service refuses premature allowance and retains its whole underfunded bill", "[heavy-onboard-service]")
{
	{
		HeavyOnboardServiceRig rig(21, 1); const auto before = ComputeSpaceManifestDigest(); CHECK(rig.Control().Failed()); CHECK(ComputeSpaceManifestDigest() == before); CHECK(rig.craft->manifest->supplies[1].TotalCount() == 2);
	}
	{
		HeavyOnboardServiceRig rig; auto &manifest = *rig.craft->manifest; REQUIRE(rig.Control().Succeeded()); rig.WorkService(37);
		// Explicit isolated material loss is unit input: the remaining original part cannot be consumed as a privileged prefix of the heavy's complete two-input bill.
		REQUIRE(HubCargoTransfer::Consume(manifest.supplies[1], 1)); const auto ticks = manifest.service_ticks; rig.WorkService(1);
		CHECK(manifest.service_ticks == ticks); CHECK(manifest.service_paused); CHECK(manifest.supplies[1].TotalCount() == 1); CHECK(manifest.supplies[2].TotalCount() == 2); CHECK(manifest.returns_remaining == 0); CHECK(manifest.services_completed == 0);
		const auto before = ComputeSpaceManifestDigest(); CHECK(rig.Control().Failed()); CHECK(ComputeSpaceManifestDigest() == before);
	}
	{
		HeavyOnboardServiceRig rig; AutoRestoreBackup placement(rig.craft->construction.placement, SpacePlacement::SurfaceStored);
		const auto before = ComputeSpaceManifestDigest(); CHECK(rig.Control().Failed()); CHECK(ComputeSpaceManifestDigest() == before);
	}
}
