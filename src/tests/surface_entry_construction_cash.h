/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_construction_cash.h Bounded actual construction capital from paid current services.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEEH.
 * The original transport, packet, cost and payment owners remain authoritative.
 * This selected consumer changes only its explicit completion predicate.
 */
#ifndef TEST_SURFACE_ENTRY_CONSTRUCTION_CASH_H
#define TEST_SURFACE_ENTRY_CONSTRUCTION_CASH_H

/** Run one exact pure funding query or separately bounded normal cash-goal interval. */
TEST_CASE("Paid current services restore construction capital only through real income", "[.surface-construction-cash]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *selected = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); REQUIRE(selected != nullptr); const std::string phase{selected};
	REQUIRE((phase == "construction-cash-query" || phase == "construction-cash-work"));
	const auto paths = GetTestCaseArtifactPaths("space", "surface-construction-cash"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve actual progress or a partial native failure without supplying money or replaying work. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves/("surface-resupply-before-"+phase+"-440.sav"));
			if (phase == "construction-cash-query") QuerySurfaceFundedFarmGoods();
			else WorkSurfaceFundedFarmGoods(false, false, Money{650000});
			SaveSurfaceResupply(paths.saves/("surface-resupply-"+phase+"-440.sav"));
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase={} supplied_stock=false supplied_work=false\n", phase); observed = true;
		} catch (...) {
			failure = std::current_exception(); SaveSurfaceResupply(paths.saves/("surface-resupply-failed-"+phase+"-440.sav"));
		}
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
#endif /* TEST_SURFACE_ENTRY_CONSTRUCTION_CASH_H */
