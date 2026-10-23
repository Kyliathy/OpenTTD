/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_restock.cpp Actual paid replacement supplies at the original first-plant Surface supplier.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CCCB/operating1.3B.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * The source/fresh machinery stay original; no carrier movement, receiver material, construction work or power is supplied here.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "plant_rebuild_material_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../surface_supply_cmd.h"
#include "../openttd.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_plant_restock_observer = nullptr; ///< Invocation-only ordinary afterload observation, never a saved stock/order/price authority.

/** @param arguments Native startup command. @return Success after observing the original normally loaded funded world. */
static bool ObservePlantRestock(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_plant_restock_observer != nullptr); (*_plant_restock_observer)(); return true;
}

/** @param paths Invocation-owned native outputs. @param world Actual original supplier/carrier/fresh machinery. @param stage Actual source0 or all-paid1. @param invoices Native new purchase debits. Save the real world without advancing simulation or changing either RNG authority. */
static void SavePlantRestock(const TestArtifactPaths &paths, const PlantRebuildCargoWorld &world, uint stage, Money invoices)
{
	const auto snapshot = CapturePlantRebuildMaterials(world, invoices); const std::array random{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	const auto path = paths.Output("saves", fmt::format("plant-rebuild-stock-{}.sav", stage));
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup format{_savegame_format, std::string{"lzma"}};
		REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
	}
	CHECK(CapturePlantRebuildMaterials(world, invoices) == snapshot); CHECK((std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random));
	fmt::print("PLANT_RESTOCK_WORLD stage={} snapshot={} path={}\n", stage, snapshot, path.string());
}

/** Exact paid replacement packets arise from native Surface orders while the original carrier stays stopped/empty and all new machinery retains zero work. */
TEST_CASE("Original rebuilt first plant buys fresh replacement materials through paid Surface orders", "[.geothermal-first-plant-rebuild-stock]")
{
	const char *input = std::getenv("OTTD_PLANT_RESTOCK_INPUT"); REQUIRE(input != nullptr);
	const auto paths = GetTestCaseArtifactPaths("geothermal", "replacement-stock"); bool observed = false; std::exception_ptr failure;
	/** Ordinary afterload supplies every real participant; only actual paid order commands create the new construction packets. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error)); auto world = LoadPlantRebuildCargoWorld();
			REQUIRE(world.carrier->IsStoppedInDepot()); REQUIRE(CargoPacket::GetNumItems() == 0); REQUIRE(TimerGameCalendar::year.base() == 2000);
			const auto tick = TimerGameTick::counter, utility = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
			const auto old = GetSurfaceSupplyOrderHistory(); const std::vector<SurfaceSupplyOrder> original{old.begin(), old.end()}; Money invoices = 0;
			SavePlantRestock(paths, world, 0, invoices);
			// Buy the complete new frozen bill through canonical native tariffs.
			// Consumed original bills stay historical; no existing order/cap or
			// retired material is reused as a free replacement supply.
			for (const auto label : PLANT_REBUILD_LABELS) {
				const auto quantity = static_cast<uint16_t>(PlantRebuildWholeBill(label)); REQUIRE(quantity > 0);
				const auto supply = ComputeSurfaceSupplyDigest(); const auto before = Company::Get(_current_company)->money; const auto packets = CargoPacket::GetNumItems();
				const std::array random{_random.state[0], _random.state[1]}; const auto quote = PrepareSurfaceSupplyOrder(world.origin, label, quantity); REQUIRE(quote.error == SurfaceSupplyError::None); REQUIRE(quote.cost > Money{0});
				const auto estimate = Command<Commands::OrderSurfaceSupply>::Do({}, world.origin, label, quantity, quote.next_order); REQUIRE(estimate.Succeeded());
				// The existing purchase owner charges its quote once; the native
				// command deliberately adds zero extra fee instead of double billing.
				CHECK(estimate.GetCost() == Money{0}); CHECK(ComputeSurfaceSupplyDigest() == supply); CHECK(Company::Get(_current_company)->money == before);
				CHECK(CargoPacket::GetNumItems() == packets); CHECK((std::array{_random.state[0], _random.state[1]} == random));
				const auto cost = Command<Commands::OrderSurfaceSupply>::Do(DoCommandFlag::Execute, world.origin, label, quantity, quote.next_order); REQUIRE(cost.Succeeded());
				CHECK(cost.GetCost() == Money{0}); CHECK(Company::Get(_current_company)->money == before - quote.cost); invoices += quote.cost;
				const auto *order = GetSurfaceSupplyOrder(quote.next_order); REQUIRE(order != nullptr); CHECK(order->quantity == quantity); CHECK(order->refunded == 0); CHECK(order->dispatched == 0);
				const auto ledger = CheckPlantRebuildMaterial(world, label); CHECK(ledger.physical == quantity); CHECK(ledger.source == quantity);
			}
			const auto current = GetSurfaceSupplyOrderHistory(); REQUIRE(current.size() == original.size() + 4);
			// Earlier paid/cap/refund histories remain exact; appending real new
			// orders cannot rewrite old dispatch or resurrect consumed material.
			for (uint index = 0; index < original.size(); ++index) CHECK(current[index] == original[index]);
			for (const CargoPacket *packet : CargoPacket::Iterate()) {
				CHECK(packet->GetCompanySupplyOrder() >= current[original.size()].id.base()); CHECK_FALSE(packet->HasCompanySupplyShipped());
				CHECK(packet->GetFirstStation() == world.origin); CHECK(packet->GetSource().type == SourceType::Industry); CHECK(packet->GetSource().id == Source::Invalid);
			}
			CHECK(CargoPacket::GetNumItems() == 4); CHECK(world.carrier->IsStoppedInDepot()); CHECK(TimerGameTick::counter == tick);
			CHECK(Company::Get(_current_company)->money == cash - invoices); CHECK(ComputeUtilityPowerDigest() == utility);
			SavePlantRestock(paths, world, 1, invoices);
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	AutoRestoreBackup observer{_plant_restock_observer, &inspect}; IConsole::CmdRegister("plant_rebuild_stock", ObservePlantRestock);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "null:ticks=2", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
