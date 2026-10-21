/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_construction_capital.h Pure actual replacement and module capital proposal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEG.
 * Estimates retain original packet, structure, hull, financial and clock ownership.
 */
#ifndef TEST_SURFACE_ENTRY_CONSTRUCTION_CAPITAL_H
#define TEST_SURFACE_ENTRY_CONSTRUCTION_CAPITAL_H
#include "../space_structure.h"
#include "../space_structure_cmd.h"

/** Price the real missing builder and existing adjacent module sites without creating native owners. */
static void QuerySurfaceConstructionCapital()
{
	VerifySurfaceFundedFarmGoods(); const auto *company = Company::Get(_current_company);
	REQUIRE(company->money == Money{101093}); REQUIRE(company->current_loan == Money{5000000});
	const auto *center = Station::Get(StationID{21}); REQUIRE(center->launch_center != nullptr);
	const auto generation = center->launch_center->generation;
	const auto builder = QuoteSurfaceResupply<Commands::AssembleLaunchCraft>(TileIndex{2891}, center->index, generation, uint16_t{20});
	REQUIRE(builder.Succeeded()); REQUIRE(builder.GetCost() == Money{100000});
	const auto *construction = GetSpaceConstructionDefinition(SpaceBuildKind::ConstructionProbe);
	std::array<uint, 2> needed{}; Money modules{0}; uint module_work = 0;
	// These detached rectangles only ask the current native placement owner.
	// They never enter the pool or serve as an invented completed Dock/Assembly.
	for (const auto [kind, tile] : {std::pair{SpaceBuildKind::Dock, TileIndex{11277}}, std::pair{SpaceBuildKind::Assembly, TileIndex{11467}}}) {
		const auto *definition = GetSpaceConstructionDefinition(kind); REQUIRE(definition != nullptr);
		const auto quote = QuoteSurfaceResupply<Commands::BuildSpaceBlueprint>(tile, tile, kind); REQUIRE(quote.Succeeded());
		REQUIRE(quote.GetCost() == Money{definition->capital});
		SpaceStructureState proposed; proposed.owner = _current_company; proposed.kind = kind;
		proposed.x = TileX(tile); proposed.y = TilePlaneY(tile);
		const auto *core = GetOrbitalConstructionCore(proposed); REQUIRE(core != nullptr); REQUIRE(IsOrbitalModuleOperational(*core));
		REQUIRE(core->state.x == 10); REQUIRE(core->state.y == 48);
		modules += quote.GetCost(); module_work += definition->work_ticks;
		// Every actual recipe input is aggregated before comparing current stock;
		// independent module estimates cannot spend the same packets twice.
		for (const auto &input : definition->inputs) {
			REQUIRE((input.label == CargoLabel{"STEL"} || input.label == CargoLabel{"GOOD"}));
			needed[input.label == CargoLabel{"STEL"} ? 0 : 1] += input.quantity;
		}
		fmt::print("SURFACE_RESUPPLY_CONSTRUCTION_CAPITAL_MODULE kind={} tile={} capital={} work={} core={} generation={} admitted=true actual_created=false\n",
			to_underlying(kind), tile.base(), quote.GetCost().base(), definition->work_ticks, core->index.base(), core->generation);
	}
	// The old retired builder's embodied inputs are terminal history. Only a
	// genuinely new recipe can supply the missing construction participant.
	for (const auto &input : construction->inputs) needed[input.label == CargoLabel{"STEL"} ? 0 : 1] += input.quantity;
	for (uint index = 0; index < 2; ++index) {
		const auto cargo = GetCargoTypeByLabel(index == 0 ? CargoLabel{"STEL"} : CargoLabel{"GOOD"});
		const uint held = center->goods[cargo].AvailableCount();
		fmt::print("SURFACE_RESUPPLY_CONSTRUCTION_CAPITAL_INPUT index={} needed={} held={} missing={} recovered_retired=0\n",
			index, needed[index], held, needed[index]-std::min(needed[index], held));
	}
	const auto &budget = GetMultiLayerSpaceBudget();
	const Money missions{2 * budget.mission_fee};
	const Money pad{static_cast<int64_t>(2 * (uint64_t{budget.prepare_ticks}+budget.turnaround_ticks)*budget.pad_per_day/74)};
	const Money work{static_cast<int64_t>((uint64_t{module_work}*budget.active_per_day+73)/74)};
	const Money reserve{150000}; const auto base = modules+builder.GetCost()+missions+pad+work+reserve;
	const Money extra_builder_work{static_cast<int64_t>((uint64_t{construction->work_ticks}*budget.active_per_day+73)/74)};
	const Money extra_builder_pad{static_cast<int64_t>((uint64_t{budget.prepare_ticks}+budget.turnaround_ticks)*budget.pad_per_day/74)};
	const auto lower = base+extra_builder_work+extra_builder_pad+Money{budget.mission_fee};
	const uint fuel = 3*(uint{budget.outbound_fuel}+budget.recovery_fuel);
	const auto propellant = center->goods[GetSpaceSupplyCargo(SpaceSupply::Propellant)].AvailableCount();
	fmt::print("SURFACE_RESUPPLY_CONSTRUCTION_CAPITAL_BUDGET cash={} debt={} working_floor=100000 builder={} builder_work={} modules={} missions={} pad={} module_work={} reserve={} base={} extra_builder_work={} extra_builder_pad={} extra_builder_mission={} minimum={} gap={} fuel={} held_fuel={} fuel_missing={} cash_milestone=650000 full_logistics_priced=false funded=false spending_permission=false future_income_counted=0 credit_counted=0\n",
		company->money.base(), company->current_loan.base(), builder.GetCost().base(), construction->work_ticks, modules.base(), missions.base(), pad.base(), work.base(), reserve.base(), base.base(),
		extra_builder_work.base(), extra_builder_pad.base(), budget.mission_fee, lower.base(), (lower-company->money).base(), fuel, propellant, fuel-std::min(fuel, propellant));
	const auto bill = QuoteSurfaceFixedPriceIncome(2000); const auto operating = bill.upkeep+bill.interest+bill.station;
	fmt::print("SURFACE_RESUPPLY_CONSTRUCTION_CAPITAL_SAVINGS ticks=2000 operating={} reserve=40000 target={} cash={} funded={} future_income_counted=0 spending_permission=false\n",
		operating.base(), (operating+Money{40000}).base(), company->money.base(), company->money >= operating+Money{40000});
}

/** Preserve every current family while asking only native estimates and detached recipe totals. */
TEST_CASE("Genuine replacement and module capital is quoted without granting construction", "[.surface-construction-capital]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); REQUIRE(selected != nullptr);
	REQUIRE(std::string_view{selected} == "construction-capital-query");
	const auto paths = GetTestCaseArtifactPaths("space", "surface-construction-capital"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Retain the exact unchanged native proposal or precise partial failure without replaying work. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves/"surface-resupply-before-construction-capital-query-440.sav"); QuerySurfaceConstructionCapital();
			SaveSurfaceResupply(paths.saves/"surface-resupply-construction-capital-query-440.sav");
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase=construction-capital-query supplied_stock=false supplied_work=false\n"); observed = true;
		} catch (...) {
			failure = std::current_exception(); SaveSurfaceResupply(paths.saves/"surface-resupply-failed-construction-capital-query-440.sav");
		}
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
#endif /* TEST_SURFACE_ENTRY_CONSTRUCTION_CAPITAL_H */
