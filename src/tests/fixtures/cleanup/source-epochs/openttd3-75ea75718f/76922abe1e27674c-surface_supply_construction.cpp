/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_construction.cpp Actual ordered purchased freight reaches native collector/riser bills through two rail transitions.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CD.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-e-first-plant-bootstrap.md for the separate later paid-excavation/operating-plant proof.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "construction_supply_fixture.h"
#include "../saveload/saveload_func.h"
#include "../core/format.hpp"
#include "../gfx_func.h"
#include "../table/sprites.h"

/**
 * Persist actual native participants without changing their positions, cargo, work or money.
 * @param rig Actual acquired world; its original-plus-native engine mapping is preserved.
 * @param name Distinct immutable output checkpoint within the generated review run.
 */
static void SaveConstructionSupply(ConstructionSupplyRig &rig, std::string_view name)
{
	LayerSaveContent content{rig.ActivatedContent(), true};
	AutoRestoreBackup local{_local_company, _current_company};
	// Standalone save tests have no graphical startup cursor. Initialize the
	// normal cursor and suppress only presentation while the actual writer runs.
	SetMouseCursor(SPR_CURSOR_MOUSE, PAL_NONE);
	AutoRestoreBackup mode{_game_mode, GameMode::Menu};
	AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	const auto paths = GetTestCaseArtifactPaths("surface-supply", "construction");
	const auto path = paths.Output("saves", fmt::format("{}.sav", name));
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	fmt::print("CONSTRUCTION_SUPPLY_SAVE name={} cash={} history={:016X} path={}\n", name,
		Company::Get(_current_company)->money.base(), ComputeSurfaceSupplyDigest(), path.string());
}

/**
 * Drive one genuine native bill from paid Surface stock to completed construction.
 * @param collector Select the collector's three labels or the riser's four labels.
 * All floor/route/portal preparation is explicit fixture baseline; no full first-plant or paid-TBM claim follows from this acceptance.
 */
static void RunConstructionSupply(bool collector)
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
	ConstructionSupplyRig rig{collector};
	const auto name = collector ? "collector" : "riser";
	const auto *bill = GetMultiLayerGeothermalConstruction(rig.kind);
	REQUIRE(rig.setup_charge > Money{bill->capital_cost});
	rig.Purchase();
	CHECK(rig.seed_charge == Money{collector ? 2520 : 2820});
	const Money settled = rig.initial_cash - rig.setup_charge - rig.seed_charge;
	CHECK(Company::Get(_current_company)->money == settled);
	REQUIRE(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
	SaveConstructionSupply(rig, fmt::format("{}-purchased", name));
	REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, rig.head->index, true).Succeeded());
	DriveConstructionSupply(*rig.head, *rig.origin, *rig.destination, rig.origin->index, false);
	// Observe a real loaded departure before either portal; checkpoint custody
	// must survive ordinary loading without manufacturing another source order.
	uint ticks = 0;
	while (rig.head->current_order.IsType(OT_LOADING) && ticks < 2000) {
		++TimerGameTick::counter; REQUIRE(rig.head->Tick()); LoadUnloadStation(rig.origin); ++ticks;
	}
	REQUIRE(ticks < 2000);
	uint loaded = 0;
	for (const Train *part = rig.head; part != nullptr; part = part->Next()) {
		loaded += part->cargo.TotalCount();
		for (const CargoPacket *packet : *part->cargo.Packets()) {
			CHECK(packet->GetCompanySupplyOrder() != 0); CHECK(packet->HasCompanySupplyShipped());
			CHECK(packet->GetFirstStation() == rig.origin->index);
		}
	}
	uint expected = 0;
	for (const auto &input : bill->inputs) expected += input.quantity;
	CHECK(loaded == expected);
	REQUIRE(GetTileLayer(rig.head->tile) == MapLayer::Surface);
	SaveConstructionSupply(rig, fmt::format("{}-outbound", name));
	const auto journey = DriveConstructionSupply(*rig.head, *rig.origin, *rig.destination, rig.destination->index, true);
	CHECK(GetTileLayer(rig.head->tile) == MapLayer::DeepUnderground);
	CHECK(Company::Get(_current_company)->money == settled);
	CHECK(rig.head->profit_this_year == Money{0});
	// Every paid label must be in the sole real accepted slot, with no stock
	// left at either station or in any vehicle. This is stronger than a receipt
	// total that could mask a duplicate packet elsewhere on the route.
	for (const auto &input : bill->inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		CHECK(rig.origin->goods[cargo].TotalCount() == 0); CHECK(rig.destination->goods[cargo].TotalCount() == 0);
		CHECK(rig.recipient->GetCargoAccepted(cargo)->waiting == input.quantity);
		CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[cargo] == input.quantity);
	}
	CHECK(CargoPacket::GetNumItems() == 0);
	REQUIRE(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
	SaveConstructionSupply(rig, fmt::format("{}-delivered", name));
	rig.Work(148);
	CHECK_FALSE((collector ? IsGeothermalCollectorOperational(rig.recipient->index) : IsGeothermalRiserOperational(rig.recipient->index)));
	SaveConstructionSupply(rig, fmt::format("{}-partial", name));
	rig.Work(148);
	CHECK((collector ? IsGeothermalCollectorOperational(rig.recipient->index) : IsGeothermalRiserOperational(rig.recipient->index)));
	for (const auto &input : bill->inputs) CHECK(rig.recipient->GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting == 0);
	// An actual empty-return round trip uses the same native orders and driven
	// arrivals. No extra visit may recover freight income, cargo or a new bill.
	DriveConstructionSupply(*rig.head, *rig.origin, *rig.destination, rig.origin->index, true);
	DriveConstructionSupply(*rig.head, *rig.origin, *rig.destination, rig.destination->index, true);
	CHECK(Company::Get(_current_company)->money == settled); CHECK(CargoPacket::GetNumItems() == 0);
	CHECK(rig.head->profit_this_year == Money{0});
	for (const auto &input : bill->inputs) {
		const auto cargo = GetCargoTypeByLabel(input.label);
		CHECK(rig.recipient->GetCargoAccepted(cargo)->waiting == 0);
		CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[cargo] == input.quantity);
	}
	for (const auto &row : GetSurfaceSupplyOrderHistory()) {
		CHECK(row.dispatched == row.quantity); CHECK(row.refunded == 0);
		CHECK(PrepareSurfaceSupplyCancellation(row.id).quantity == 0);
	}
	SaveConstructionSupply(rig, fmt::format("{}-complete", name));
	fmt::print("CONSTRUCTION_SUPPLY_RECEIPT kind={} units={} seed={} setup={} cash={} journey_ticks={} industry={} train={} origin={} destination={}\n",
		name, expected, rig.seed_charge.base(), rig.setup_charge.base(), settled.base(), journey, rig.recipient->index.base(), rig.head->index.base(),
		rig.origin->index.base(), rig.destination->index.base());
}

/** Actual purchased collector inputs travel through Surface/Underground/Deep; native construction consumes them once. */
TEST_CASE("Paid Surface collector bill arrives by native ordered conventional train", "[surface-supply-construction-collector]")
{
	RunConstructionSupply(true);
}

/** All four paid riser inputs travel through both physical rail transitions and remain non-paying at final construction intake. */
TEST_CASE("Paid Surface riser bill arrives by native ordered conventional train", "[surface-supply-construction-riser]")
{
	RunConstructionSupply(false);
}
