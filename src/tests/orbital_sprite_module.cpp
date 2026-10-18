/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_sprite_module.cpp Read-only native module stages, paid work and sorting boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K
 * Consumed component frames and supplied packets are declared fixture inputs;
 * original public owners earn every work step. These are not ordinary-game procurement proofs.
 */
#include "../stdafx.h"
#include "manufacturing_fixture.h"
#include "../orbital_sprite.h"
#include "../orbital_service.h"
#include "../safeguards.h"

/** Require the actual cell's published source pose.
 * @param site Original native module owner.
 * @param tile Original row-major world cell.
 * @param part Native back or genuine near-jaw foreground.
 * @return Successfully resolved transient source pose.
 */
static OrbitalArtPose ModulePose(const SpaceStructure &site, uint8_t tile = 0, uint8_t part = 0)
{
	const auto pose = GetOrbitalModuleArtPose(site, _current_company, tile, part);
	REQUIRE(pose); REQUIRE(ResolveOrbitalSprite(*pose)); return *pose;
}

/** Every declared family uses actual fractional construction progress; packets alone erect no hardware. */
TEST_CASE("Orbital module source stages preserve physical work and unsupported tank activation", "[orbital-art-module]")
{
	ManufacturingMaterializationRig rig;
	auto &site = *rig.assembly;
	// Each iteration explicitly varies a component's source identity only. No
	// changed-kind frame is accepted as a paid save or operational producer.
	for (const auto kind : {SpaceBuildKind::Core, SpaceBuildKind::Dock, SpaceBuildKind::Storage, SpaceBuildKind::Power,
		SpaceBuildKind::FuelStore, SpaceBuildKind::Assembly, SpaceBuildKind::Steelworks, SpaceBuildKind::Goodsworks,
		SpaceBuildKind::Refinery, SpaceBuildKind::IceProcessor}) {
		CAPTURE(kind); AutoRestoreBackup state(site.state, site.state);
		AutoRestoreBackup ticks(site.job.work_ticks, uint32_t{0});
		AutoRestoreBackup fraction(site.job.work_fraction, uint16_t{0});
		AutoRestoreBackup remainder(site.job.work_remainder, uint16_t{0});
		site.state.kind = kind; site.state.phase = SpaceStructurePhase::Blueprint;
		CHECK(ModulePose(site).state == 0);
		site.job.work_remainder = 1; CHECK(ModulePose(site).state == 1); site.job.work_remainder = 0;
		site.job.work_fraction = 1; CHECK(ModulePose(site).state == 1); site.job.work_fraction = 0;
		site.job.work_ticks = 1; CHECK(ModulePose(site).state == 1);
		const auto *definition = GetSpaceConstructionDefinition(kind); REQUIRE(definition);
		const auto cells = definition->width * definition->height;
		CHECK_FALSE(GetOrbitalModuleArtPose(site, _current_company, cells, 0));
		CHECK_FALSE(GetOrbitalModuleArtPose(site, _current_company, 0, 2));
		CHECK_FALSE(GetOrbitalModuleArtBounds(site, cells, 0));
		CHECK_FALSE(GetOrbitalModuleArtBounds(site, 0, 2));
		site.state.phase = SpaceStructurePhase::Cancelled;
		CHECK_FALSE(GetOrbitalModuleArtPose(site, _current_company, 0, 0));
		if (kind == SpaceBuildKind::FuelStore) {
			site.state.phase = SpaceStructurePhase::FrameComplete;
			CHECK(ModulePose(site).state == 2); CHECK(ModulePose(site).frame == 0);
			CHECK_FALSE(IsSupportedOrbitalModuleKind(kind));
		}
	}
}

/** Original complete hardware remains distinct from independent inventory and service/power admission. */
TEST_CASE("Orbital idle modules do not invent work or storage electricity demand", "[orbital-art-module]")
{
	ManufacturingMaterializationRig rig;
	CHECK(ModulePose(*rig.assembly).state == 2); CHECK(ModulePose(*rig.assembly).frame == 0);
	CHECK(ModulePose(*rig.storage).state == 2);
	REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, rig.core->index, rig.core->generation, true).Succeeded());
	CHECK(ModulePose(*rig.core).state == 3);
	CHECK(ModulePose(*rig.storage).state == 2); // Storage has a Core dependency, not a service consumer.
	CHECK(ModulePose(*rig.assembly).state == 2); // No pending request is starved by absent demand.
	rig.assembly->module->activation_ticks = 1; CHECK(ModulePose(*rig.assembly).state == 3);
}

/** Actual existing hull clearance sorts genuine jaw pixels while each native source anchor stays fixed. */
TEST_CASE("Orbital per-cell foreground bounds retain world anchors and physical approach clearance", "[orbital-art-module]")
{
	ManufacturingMaterializationRig rig;
	// Enumerate both delivered footprint sizes. Only native per-cell source IDs
	// vary; the approach clearance belongs to the original whole site.
	for (const auto *site : {rig.core, rig.assembly}) {
		const auto &definition = *GetSpaceConstructionDefinition(site->state.kind);
		for (uint8_t tile = 0; tile < definition.width * definition.height; ++tile) {
			const auto back = GetOrbitalModuleArtBounds(*site, tile, 0);
			const auto front = GetOrbitalModuleArtBounds(*site, tile, 1);
			REQUIRE(back); REQUIRE(front);
			CHECK(front->origin.x + front->offset.x == 0);
			CHECK(front->origin.y + front->offset.y == 0);
			CHECK(front->origin.z + front->offset.z == 0);
			CHECK(front->origin.z == 8); CHECK(front->extent.z == 1);
			CHECK((tile / definition.width) * TILE_SIZE + front->origin.y == definition.height * TILE_SIZE + 4);
			CHECK((tile % definition.width) * TILE_SIZE + front->origin.x == 4);
			CHECK(ModulePose(*site, tile, 0).tile == tile);
			CHECK(ModulePose(*site, tile, 1).part == 1);
		}
	}
}

/** One real750J opportunity alone selects activity; pause and physical power loss keep original work. */
TEST_CASE("Orbital Assembly source follows actual native paid work and typed waits", "[orbital-art-module]")
{
	ManufacturingMaterializationRig rig; AutoRestoreBackup pause(_pause_mode, PauseModes{});
	auto &job = rig.Start(15);
	CHECK(ModulePose(*rig.assembly).frame == 0);
	rig.Tick(); REQUIRE(job.work_ticks == 1);
	CHECK(GetOrbitalManufactureWait(*rig.assembly) == OrbitalManufactureWait::None);
	CHECK(ModulePose(*rig.assembly).state == 2); CHECK(ModulePose(*rig.assembly).frame == 1);
	_pause_mode.Set(PauseMode::Normal); CHECK(ModulePose(*rig.assembly).frame == 0);
	CHECK(job.work_ticks == 1); _pause_mode.Reset(PauseMode::Normal);
	REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, rig.core->index, rig.core->generation, true).Succeeded());
	CHECK(GetOrbitalManufactureWait(*rig.assembly) == OrbitalManufactureWait::Power);
	CHECK(ModulePose(*rig.assembly).state == 3); CHECK(job.work_ticks == 1);
	REQUIRE(CmdSetOrbitalPanelPaused(DoCommandFlag::Execute, rig.core->index, rig.core->generation, false).Succeeded());
	{
		// Exhaust the actual company's unit admission as a declared boundary
		// input. Finished paid work and every input must remain held, and a
		// read-only display quote cannot create a product or change the owners.
		auto &units = Company::Get(_current_company)->group_all[VehicleType::Space].num_vehicle;
		AutoRestoreBackup unit_bound(units, static_cast<std::remove_reference_t<decltype(units)>>(MAX_SPACE_VEHICLES));
		rig.Work(GetSpaceVehicleConstruction(15)->work_ticks - 1);
		REQUIRE(job.phase == OrbitalManufacturePhase::AwaitingMaterialization);
		const auto held = ComputeSpaceStructureDigest(); const auto power = ComputeUtilityPowerDigest();
		CHECK(GetOrbitalManufactureWait(*rig.assembly) == OrbitalManufactureWait::Output);
		CHECK(ModulePose(*rig.assembly).state == 3); CHECK(ModulePose(*rig.assembly).frame == 0);
		CHECK(GetSpaceVehicleCount() == 0);
		CHECK(ComputeSpaceStructureDigest() == held); CHECK(ComputeUtilityPowerDigest() == power);
	}
	rig.Tick();
	REQUIRE(job.phase == OrbitalManufacturePhase::Complete); CHECK(GetSpaceVehicleCount() == 1);
	CHECK(GetOrbitalManufactureWait(*rig.assembly) == OrbitalManufactureWait::None);
	CHECK(ModulePose(*rig.assembly).state == 2); CHECK(ModulePose(*rig.assembly).frame == 0);
}

/** Repeated owner and spectator views cannot advance native work, cargo, money, epochs or RNG. */
TEST_CASE("Orbital module rendering preserves original native owners and actor boundaries", "[orbital-art-module]")
{
	ManufacturingMaterializationRig rig; AutoRestoreBackup pause(_pause_mode, PauseModes{});
	auto &job = rig.Start(15); rig.Tick();
	const auto structures = ComputeSpaceStructureDigest(); const auto utility = ComputeUtilityPowerDigest();
	const auto random = _random, interactive = _interactive_random;
	const auto money = Company::Get(_current_company)->money;
	const auto actor = _current_company; const auto tick = TimerGameTick::counter; const auto work = job.work_ticks;
	// Vary independent source cells/parts and foreign observers through the
	// actual public read paths; no query may change original actor authority.
	for (uint repeat = 0; repeat < 32; ++repeat) {
		CHECK(ModulePose(*rig.assembly, repeat % 4, repeat % 2).frame == 1);
		const auto foreign = GetOrbitalModuleArtPose(*rig.assembly, CompanyID::Invalid(), repeat % 4, repeat % 2);
		REQUIRE(foreign); CHECK(foreign->frame == 0); CHECK(foreign->state == 2);
	}
	CHECK(ComputeSpaceStructureDigest() == structures); CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
	CHECK(Company::Get(actor)->money == money);
	CHECK(_current_company == actor); CHECK(TimerGameTick::counter == tick); CHECK(job.work_ticks == work);
}
