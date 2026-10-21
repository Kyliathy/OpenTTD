/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_constructor_retirement.h Actual public retirement of the genuine empty home builder.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZVED.
 * Native sale alone banks capital. Consumed4Steel8Goodsare never recovered,
 * and a future replacement/launch remains a separate genuinely funded producer.
 */
#ifndef TEST_SURFACE_ENTRY_CONSTRUCTOR_RETIREMENT_H
#define TEST_SURFACE_ENTRY_CONSTRUCTOR_RETIREMENT_H

/** Retire the exact actual empty home incarnation once and freshly price only real post-sale cash. */
static void PaySurfaceConstructorRetirement()
{
	const auto *craft = VerifySurfaceConstructorHome(true); const auto *company = Company::Get(_current_company);
	REQUIRE(SurfaceConstructorReachedHome(craft)); REQUIRE(craft->vehstatus.Test(VehState::Stopped));
	REQUIRE(company->money == Money{41417}); REQUIRE(company->current_loan == Money{5000000});
	REQUIRE(company->money >= Money{40000}); const auto tick = TimerGameTick::counter;
	const auto before = company->money; const auto value = craft->value; REQUIRE(value == Money{29927});
	const auto quote = QuoteSurfaceResupply<Commands::SellVehicle>(craft->tile, craft->index, false, false, ClientID::Server);
	REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == -value);
	const auto tile = craft->tile;
	const auto paid = PaySurfaceResupply<Commands::SellVehicle>(tile, craft->index, false, false, ClientID::Server);
	REQUIRE(paid.GetCost() == -value); REQUIRE(ResolveSpaceCraft({VehicleID{38}, uint64_t{2}}) == nullptr);
	REQUIRE(company->money == before+value); REQUIRE(TimerGameTick::counter == tick); VerifySurfaceFarmBackhaul(true);
	const auto *rocket = SpaceVehicle::Get(VehicleID{54}); REQUIRE(rocket->generation == 1);
	REQUIRE(rocket->construction.placement == SpacePlacement::SurfaceStored); REQUIRE(rocket->GetRunningCost() == Money{0});
	fmt::print("SURFACE_RESUPPLY_CONSTRUCTOR_RETIREMENT_PAID vehicle=38 generation=2 model=20 invoice={} refund_banked={} cash={} elapsed=0 reserve=40000 original_capital=100000 retired_steel=4 retired_goods=8 components_recovered=0 unused_returns_extinguished=2 freight_income=0 future_idle_per_day_removed=100 avoided_cost_realized=0 replacement_bought=false incomplete_base=610000 replacement_launch_priced=false\n",
		paid.GetCost().base(), value.base(), company->money.base());
	// Only after native destruction and the banked refund does the unchanged
	// owner omit the absent builder. No forecast earns an idle saving or fare;
	// all continuing actual services and monthly charges still require funding.
	for (const uint limit : {2000, 1000, 500, 250, 64}) {
		const auto bill = QuoteSurfaceFixedPriceIncome(limit); const auto operating = bill.upkeep+bill.interest+bill.station;
		const auto target = operating+Money{40000};
		fmt::print("SURFACE_RESUPPLY_CONSTRUCTOR_RETIREMENT_ADMISSION ticks={} operating={} reserve=40000 target={} cash={} funded={} future_income_counted=0 avoided_cost_realized=0 unbanked_refund_counted=0 work_permission=false\n",
			limit, operating.base(), target.base(), company->money.base(), company->money >= target);
	}
}

/** Execute only one exact actual home sale and retain any partial native failure without replay. */
TEST_CASE("Actual empty home builder retirement banks only its native refund", "[.surface-constructor-retirement]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const char *phase = std::getenv("OTTD_SURFACE_RESUPPLY_PHASE"); REQUIRE(phase != nullptr); REQUIRE(std::string_view{phase} == "constructor-retirement-pay");
	const auto paths = GetTestCaseArtifactPaths("space", "surface-constructor-retirement"); const auto config = GetTestRuntimeConfig();
	bool observed = false; std::exception_ptr failure;
	/** Preserve the exact original sale or partial native boundary; never recreate the retired hull. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company); REQUIRE(Company::IsValidID(_current_company));
			SaveSurfaceResupply(paths.saves/"surface-resupply-before-constructor-retirement-pay-440.sav"); PaySurfaceConstructorRetirement();
			SaveSurfaceResupply(paths.saves/"surface-resupply-constructor-retirement-pay-440.sav");
			fmt::print("SURFACE_RESUPPLY_COMPLETE phase=constructor-retirement-pay supplied_stock=false supplied_work=false\n"); observed = true;
		} catch (...) {
			failure = std::current_exception(); SaveSurfaceResupply(paths.saves/"surface-resupply-failed-constructor-retirement-pay-440.sav");
		}
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_resupply_observer, &inspect); IConsole::CmdRegister("surface_entry_resupply", ObserveSurfaceResupply);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_capital_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
#endif /* TEST_SURFACE_ENTRY_CONSTRUCTOR_RETIREMENT_H */
