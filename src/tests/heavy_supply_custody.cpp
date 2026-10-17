/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file heavy_supply_custody.cpp Heavy source-era supply admission and actual factory-product pod location contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CB
 * Copied shell history and isolated frame/stock inputs are declared unit data; loaded-world acceptance separately proves actual paid physical custody.
 */
#include "../stdafx.h"
#include "manufacturing_fixture.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_mining_tuning.h"
#include "../safeguards.h"

/** Earlier21 commissioning/mining files retain exhausted allowance without borrowing later shells, parts or service history. */
TEST_CASE("Heavy supply source admission preserves older empty manifests and denies future assets", "[heavy-supply-custody]")
{
	SpaceManifestRecord record; record.craft = {VehicleID{22}, 8}; record.returns_remaining = 1;
	CHECK(IsSpaceHeavyResupplyStateSupported(21, record, false)); record.returns_remaining = 0; CHECK(IsSpaceHeavyResupplyStateSupported(21, record, false));
	// Each independently introduced physical/history field must refuse in an older mining-only source; current schema support still needs normal native/copy owner joins.
	for (uint field = 0; field < 7; ++field) {
		auto changed = record;
		switch (field) {
			case 0: changed.pods.push_back({SpacePodID{3}, 9}); break;
			case 1: changed.pending_returns = 1; break;
			case 2: changed.service_ticks = 1; break;
			case 3: changed.services_completed = 1; break;
			default: changed.supplies[field - 4].push_back({CargoPacketID{5}, StationID::Invalid()}); break;
		}
		CHECK_FALSE(IsSpaceHeavyResupplyStateSupported(21, changed, false)); CHECK(IsSpaceHeavyResupplyStateSupported(21, changed, true) == (field == 0 || field >= 4));
		CHECK(IsSpaceHeavyResupplyStateSupported(15, changed, false)); // Existing roles retain their own old source admission; this helper grants them no new stock or work.
	}
	CHECK(GetSpaceSupplyCapacity(21, SpaceSupply::RefinedMetal) == 8); CHECK(GetSpaceSupplyCapacity(21, SpaceSupply::Electronics) == 4); CHECK(GetSpaceSupplyCapacity(21, SpaceSupply::Propellant) == 0);
}

/** Genuine locally manufactured roles retain a shell's real historical Earth tile; arbitrary missing carrier locations cannot become invented homes. */
TEST_CASE("Paid orbital factory products preserve a shell Surface history without fabricating a carrier home", "[heavy-supply-custody]")
{
	// Each role gets a fresh isolated factory, so the previous paid product cannot occupy this role's output berth. Both small factory products and newly supplied21 lack a native Earth storage tile.
	for (uint16_t model : {uint16_t{15}, uint16_t{21}}) {
		CAPTURE(model); ManufacturingMaterializationRig rig;
		AutoRestoreBackup weight(CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"NORE"}))->weight, uint8_t{16}); // Declared unit ore dimension; no native gameplay stock or packet is created here.
		SpacePodState shell; shell.surface_tile = TileXYL(20, 20, MapLayer::Surface);
		auto &job = rig.Start(model); rig.Work(GetSpaceVehicleConstruction(model)->work_ticks); REQUIRE(job.phase == OrbitalManufacturePhase::Complete);
		const auto *craft = ResolveSpaceCraft(job.product); REQUIRE(craft != nullptr); REQUIRE(craft->construction.surface_tile == INVALID_TILE);
		const auto actual = craft->construction; CHECK(IsSpacePodCarrierSurfaceTileValid(shell, actual)); CHECK(craft->construction.surface_tile == INVALID_TILE);
		const auto original = shell.surface_tile;
		auto absent = actual; absent.assembly = {}; CHECK_FALSE(IsSpacePodCarrierSurfaceTileValid(shell, absent));
		absent = actual; absent.assembly_serial = 0; CHECK_FALSE(IsSpacePodCarrierSurfaceTileValid(shell, absent));
		absent = actual; absent.content_version = 0; CHECK_FALSE(IsSpacePodCarrierSurfaceTileValid(shell, absent));
		absent = actual; absent.placement = SpacePlacement::SurfaceStored; CHECK_FALSE(IsSpacePodCarrierSurfaceTileValid(shell, absent));
		auto surface = actual; surface.surface_tile = original; CHECK(IsSpacePodCarrierSurfaceTileValid(shell, surface));
		surface.surface_tile = TileIndex{original.base() + 1}; CHECK_FALSE(IsSpacePodCarrierSurfaceTileValid(shell, surface)); CHECK(shell.surface_tile == original);
		REQUIRE(CmdCancelOrbitalManufacture(DoCommandFlag::Execute, rig.assembly->index, rig.assembly->generation, rig.core->index, rig.core->generation).Succeeded());
	}
}
