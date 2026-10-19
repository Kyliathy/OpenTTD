/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_work.h Read-only financial observation around genuine bounded native work.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BID/15BIE.
 * Original commands, StateGameLoop and company books retain every owner. This
 * helper records deltas before rolling historical windows can discard them.
 */
#ifndef TEST_SURFACE_ENTRY_WORK_H
#define TEST_SURFACE_ENTRY_WORK_H

#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../cargotype.h"
#include "../command_func.h"
#include "../misc_cmd.h"
#include "../openttd.h"
#include "../timer/timer_game_tick.h"
#include <thread>

/** Ephemeral original work/payment observations; never another balance or source of simulation time. */
struct SurfaceEntryWorkReceipt {
	uint64_t ticks = 0; ///< Actual earned TimerGameTick delta, excluding native worker pauses.
	bool reached = false; ///< Whether the caller's original owner predicate passed.
	Money income{0}; ///< Native paid freight income accrued across quarter rollovers.
	Expenses expenses{}; ///< Native expense-class deltas accrued across year rollovers.
	CargoArray delivered{}; ///< Original delivered-cargo deltas accrued across quarter rollovers.
};

/** Earn a short genuine work/movement interval and reconcile all original economic charges.
 * @param prefix Native diagnostic family for this bounded producer.
 * @param limit Maximum actual earned ticks before retaining a resumable partial checkpoint.
 * @param done Read-only original construction/cargo/vehicle completion predicate.
 * @return Actual native tick/payment observations and whether the real goal was reached.
 */
static SurfaceEntryWorkReceipt EarnSurfaceEntryWork(std::string_view prefix, uint32_t limit, const std::function<bool()> &done)
{
	SurfaceEntryWorkReceipt result;
	const auto *company = Company::Get(_current_company); const auto cash = company->money, debt = company->current_loan;
	const auto initial = TimerGameTick::counter; const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(85);
	if (_pause_mode.Test(PauseMode::Normal)) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
	// Native work alone advances construction, movement, cargo, interest and
	// production. Reaching the actual owner predicate ends this interval; a
	// short tick limit retains partial real progress for the next native burst.
	while (TimerGameTick::counter - initial < limit) {
		if (done()) break;
		REQUIRE(std::chrono::steady_clock::now() < deadline);
		const auto year = TimerGameEconomy::year; const auto month = TimerGameEconomy::month;
		const auto expenses = company->yearly_expenses[0]; const auto quarter = company->cur_economy;
		StateGameLoop();
		const bool year_roll = TimerGameEconomy::year != year;
		const bool quarter_roll = year_roll || TimerGameEconomy::month / 3 != month / 3;
		// Each original rollover is observed on its actual tick before a
		// limited annual/quarter history can be mistaken for lifetime income.
		for (uint index = 0; index < to_underlying(ExpensesType::End); ++index) {
			const auto type = static_cast<ExpensesType>(index);
			result.expenses[type] += company->yearly_expenses[year_roll ? 1 : 0][type] - expenses[type];
			if (year_roll) result.expenses[type] += company->yearly_expenses[0][type];
		}
		const auto &completed = quarter_roll ? company->old_economy[0] : company->cur_economy;
		result.income += completed.income - quarter.income;
		if (quarter_roll) result.income += company->cur_economy.income;
		for (CargoType cargo = CargoType{0}; cargo < NUM_CARGO; ++cargo) {
			result.delivered[cargo] += completed.delivered_cargo[cargo] - quarter.delivered_cargo[cargo];
			if (quarter_roll) result.delivered[cargo] += company->cur_economy.delivered_cargo[cargo];
		}
		if (_pause_mode.Any()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	result.ticks = TimerGameTick::counter - initial; result.reached = done(); Money total{0};
	for (uint index = 0; index < to_underlying(ExpensesType::End); ++index) {
		const auto type = static_cast<ExpensesType>(index); total += result.expenses[type];
		fmt::print("{}_EXPENSE type={} amount={}\n", prefix, index, result.expenses[type].base());
	}
	REQUIRE(company->current_loan == debt); REQUIRE(company->money == cash - total);
	fmt::print("{}_EARNED earned={} reached={} cash_before={} cash={} debt={} income={} net_expense={}\n", prefix,
		result.ticks, result.reached, cash.base(), company->money.base(), debt.base(), result.income.base(), total.base());
	for (CargoType cargo = CargoType{0}; cargo < NUM_CARGO; ++cargo) {
		if (result.delivered[cargo] != 0) fmt::print("{}_DELIVERED label={} amount={}\n", prefix, CargoSpec::Get(cargo)->label.AsString(), result.delivered[cargo]);
	}
	return result;
}

#endif /* TEST_SURFACE_ENTRY_WORK_H */
