/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_paid_recovery.h Conditional genuine producer windows and actual working cash.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGJ.
 * Every further window requires a new whole cost quote from then-current cash.
 * Previous production/load/profit never becomes an advance funding credit.
 */
#ifndef TEST_SURFACE_ENTRY_PAID_RECOVERY_H
#define TEST_SURFACE_ENTRY_PAID_RECOVERY_H

/** Quote an entire next conditional window from actual present cash.
 * @param limit Whole6000proposal or a separately admitted narrower3000after refusal.
 */
static void QuerySurfacePaidRecovery(uint limit = 6000)
{
	// Only the whole original proposal or its explicitly narrowed single
	// interval may be priced; arbitrary horizon growth is outside this gate.
	REQUIRE((limit == 3000 || limit == 6000));
	const auto *company = Company::Get(_current_company); REQUIRE(company->money < Money{100000});
	REQUIRE(company->current_loan == Money{5000000}); REQUIRE(company->GetMaxLoan() == Money{5000000});
	ObserveSurfaceFarmCycle("query"); QuoteSurfaceFixedPriceIncome(3000); const auto work = QuoteSurfaceModuleCapitalWork(0, limit);
	const auto operating = work.upkeep + work.interest + work.station;
	for (const Money reserve : {Money{100000}, Money{40000}}) {
		const auto target = reserve + operating;
		fmt::print("SURFACE_RESUPPLY_RECOVERY_ADMISSION ticks={} interval_limit=3000 days={} months={} year_boundary={} upkeep={} interest={} station_charge={} operating={} reserve={} target={} cash={} funded={} purchases=0 future_income_counted=0 refunds_counted=0 credit=0\n",
			limit, work.days, work.months, work.year_boundary, work.upkeep.base(), work.interest.base(), work.station.base(), operating.base(), reserve.base(), target.base(), company->money.base(), company->money >= target);
	}
}

/** Earn only one fresh interval, stopping immediately on actual working cash. */
static void WorkSurfacePaidRecovery()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money < Money{100000});
	ObserveSurfaceFarmCycle("before"); const auto limit = SurfaceChemicalWorkLimit();
	QuoteSurfaceFarmUpgrade("paid-recovery", limit, 0, Money{0}, false);
	/** @return Whether actual paid cash has restored the working reserve. */
	const auto receipt = EarnSurfaceEntryWork("SURFACE_RESUPPLY", limit, [&] {
		REQUIRE(company->money >= Money{40000}); return company->money >= Money{100000};
	});
	ObserveSurfaceFarmCycle("after");
	fmt::print("SURFACE_RESUPPLY_RECOVERY_PROGRESS ticks={} cash={} restored={} partial={} working_shortfall={} module_base_shortfall={} module_capital_complete=false\n",
		receipt.ticks, company->money.base(), receipt.reached, !receipt.reached,
		std::max(Money{0}, Money{100000} - company->money).base(), std::max(Money{0}, Money{510000} - company->money).base());
}
#endif /* TEST_SURFACE_ENTRY_PAID_RECOVERY_H */
