/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_surplus_recovery.h Actual zero-time surplus-only native capital recovery.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVEB.
 * The public sale alone banks its refund. Parked zero-rate wagons grant no
 * avoided operating cost; actual new cash is priced before any later work.
 */
#ifndef TEST_SURFACE_ENTRY_SURPLUS_RECOVERY_H
#define TEST_SURFACE_ENTRY_SURPLUS_RECOVERY_H

/** Retire only actual empty additional58..59and report real post-sale funding without earning work. */
static void PaySurfaceSurplusRecovery()
{
	QuerySurfaceSurplusHome(); const auto *company = Company::Get(_current_company);
	const auto before = company->money; REQUIRE(before >= Money{40000}); const auto tick = TimerGameTick::counter;
	const auto paid = PaySurfaceResupply<Commands::SellVehicle>(TileIndex{1638}, VehicleID{58}, true, false, ClientID::Server);
	REQUIRE(paid.GetCost() == Money{-1504}); REQUIRE(company->money == before+Money{1504});
	VerifySurfaceFarmBackhaul(true); REQUIRE(TimerGameTick::counter == tick);
	fmt::print("SURFACE_RESUPPLY_SURPLUS_RECOVERY_PAID head=58 tail=59 removed=2 invoice=-1504 refund_banked=1504 cash={} elapsed=0 original_wagons_owned=32 active_livestock=200 active_grain=90 active_goods=50 running_cost_saved=0 reserve=40000\n", company->money.base());
	// Every later interval is priced from actual banked cash. No work, home
	// order, builder retirement or projected builder value belongs to this sale.
	for (const uint limit : {1000, 500, 250, 64}) {
		const auto bill = QuoteSurfaceFixedPriceIncome(limit); const auto operating = bill.upkeep+bill.interest+bill.station;
		const auto target = operating+Money{40000};
		fmt::print("SURFACE_RESUPPLY_SURPLUS_RECOVERY_ADMISSION ticks={} operating={} reserve=40000 target={} cash={} funded={} future_income_counted=0 avoided_cost_counted=0 unbanked_refund_counted=0 work_permission=false\n",
			limit, operating.base(), target.base(), company->money.base(), company->money >= target);
	}
}

/** Execute the exact surplus-only public sale once and retain any partial native failure. */
TEST_CASE("Only actual empty surplus wagons bank recovered capital", "[.surface-surplus-recovery]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *phase = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); REQUIRE(phase != nullptr); REQUIRE(std::string_view{phase} == "surplus-recovery-pay");
	const auto paths = GetTestCaseArtifactPaths("space", "surface-surplus-recovery"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve the exact original paid boundary, including any partial actual command failure. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves/"surface-resupply-before-surplus-recovery-pay-440.sav"); PaySurfaceSurplusRecovery();
			SaveSurfaceResupply(paths.saves/"surface-resupply-surplus-recovery-pay-440.sav");
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase=surplus-recovery-pay supplied_stock=false supplied_work=false\n"); observed = true;
		} catch (...) {
			failure = std::current_exception(); SaveSurfaceResupply(paths.saves/"surface-resupply-failed-surplus-recovery-pay-440.sav");
		}
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
#endif /* TEST_SURFACE_ENTRY_SURPLUS_RECOVERY_H */
