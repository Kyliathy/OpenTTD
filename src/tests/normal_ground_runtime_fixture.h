/* This file is part of OpenTTD. Licensed under the GNU GPL version 2. */
/** @file normal_ground_runtime_fixture.h Original-world native accounting and callback transport.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md
 * Extracted from the donor's genuine paid-world callbacks; no world construction,
 * company funding, stock assignment or private simulation tick is included.
 */
#ifndef TESTS_NORMAL_GROUND_RUNTIME_FIXTURE_H
#define TESTS_NORMAL_GROUND_RUNTIME_FIXTURE_H
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../openttd.h"
#include "../network/network.h"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../saveload/saveload_func.h"
#include "../economy_func.h"
#include "../excavation.h"
#include "../industry.h"
#include "../spoil_receiver.h"
#include "../tbm.h"
#include "../tbm_cmd.h"
#include "../tbm_exchange.h"
#include "../underground_vehicle.h"
#include "../transport_tube.h"
#include "../transport_tube_cmd.h"
#include "../utility_power.h"
#include "../engine_base.h"
#include "../engine_func.h"
#include "../misc_cmd.h"
#include "../order_cmd.h"
#include "../station_base.h"
#include "../industry_map.h"
#include "../train.h"
#include "../train_cmd.h"
#include "../vehicle_cmd.h"
#include "../track_func.h"
#include "../direction_func.h"
#include "../timer/timer.h"
#include "../timer/timer_game_tick.h"
#include "../timer/timer_game_economy.h"
#include "../timer/timer_game_realtime.h"
#include <charconv>
#include <set>
#include <map>
#include "../safeguards.h"
inline std::function<void()> *_normal_ground_newgame_observer = nullptr; ///< Invocation callback only; no saved state.

/** Dispatch the selected startup observation without constructing a world.
 * @param arguments Ordinary native startup command arguments.
 * @return True after invoking the registered real-world observer.
 */
inline bool ObserveNormalGroundNewgame(std::span<std::string_view> arguments)
{
    if (arguments.empty()) return true;
    if (_normal_ground_newgame_observer != nullptr) (*_normal_ground_newgame_observer)();
    return true;
}

/** Observe the geology owner across both physical work layers without changing terrain.
 * @return Counts indexed by the native excavation state enum.
 */
inline std::array<size_t, 4> CountNormalGroundRockStates()
{
    std::array<size_t, 4> result{};
    // Both work layers retain independent rock ownership. Traverse their native
    // geometry rather than deriving a layer from the visible camera or world Y.
    for (const auto layer : {MapLayer::Underground, MapLayer::DeepUnderground}) {
        for (const auto tile : MultiLayerMap::IterateLayer(layer)) {
            if (IsInnerTile(tile)) ++result[to_underlying(GetGeologyWorldState().terrain.Get(tile).value())];
        }
    }
    return result;
}

/** Execute a real public command and reconcile its independently quoted charge.
 * @tparam command Canonical native command whose owner performs settlement.
 * @tparam First Native command's first argument type.
 * @tparam Rest Remaining native argument types.
 * @param label Actual invoice action printed into this process's transcript.
 * @param first Native location or first argument, preserving command flags.
 * @param rest Remaining real command arguments; no fabricated cost or stock.
 */
template <Commands command, typename First, typename... Rest>
inline void PayNormalFreightCommand(std::string_view label, First first, Rest... rest)
{
	const auto cost = [&]() {
		if constexpr (GetCommandFlags<command>().Test(CommandFlag::Location)) {
			return ExtractCommandCost(Command<command>::Do({}, rest...));
		} else {
			return ExtractCommandCost(Command<command>::Do({}, first, rest...));
		}
	}();
	INFO(label);
	REQUIRE(cost.Succeeded());
	const auto *company = Company::Get(_current_company);
	const auto before = company->money;
	REQUIRE(Command<command>::Post(first, rest...));
	CHECK(company->money == before - cost.GetCost());
	fmt::print("NORMAL_FREIGHT_INVOICE action={} cost={} cash={}\n", label, cost.GetCost().base(), company->money.base());
}


/** Observe all native company ledger categories without owning or assigning money. */
struct NormalFreightEarnings {
	CompanyID owner;
	int64_t quarter;
	Money seen_income, seen_expenses;
	uint64_t seen_delivered;
	Money income = 0, expenses = 0;
	uint64_t delivered = 0;
	int32_t year;
	Expenses seen_ledger, ledger{};
	CargoArray seen_cargo;
	std::array<uint64_t, NUM_CARGO> delivered_by_cargo{};

	/** @param company Actual original company with its native transport ledger. */
	NormalFreightEarnings(const Company &company) : owner(company.index), quarter(Quarter()),
		seen_income(company.cur_economy.income), seen_expenses(company.cur_economy.expenses),
		seen_delivered(company.cur_economy.delivered_cargo.GetSum<uint64_t>()), year(TimerGameEconomy::year.base()),
		seen_ledger(company.yearly_expenses[0]), seen_cargo(company.cur_economy.delivered_cargo) {}

	/** @return Native economy quarter;calendar speed never substitutes for economy time. */
	static int64_t Quarter() { return static_cast<int64_t>(TimerGameEconomy::year.base()) * 4 + TimerGameEconomy::month / 3; }

	/** Observe actual native entries,including the final prior-quarter record after rotation. */
	void Observe()
	{
		const auto *company = Company::Get(owner);
		const auto current_quarter = Quarter();
		if (current_quarter != quarter) {
			REQUIRE(current_quarter == quarter + 1);
			const auto &previous = company->old_economy[0];
			income += previous.income - seen_income;
			expenses += previous.expenses - seen_expenses;
			delivered += previous.delivered_cargo.GetSum<uint64_t>() - seen_delivered;
			// Preserve each cargo category across economy rotation; totals cannot hide a changed cargo owner.
			for (uint type = 0; type < NUM_CARGO; ++type) {
				delivered_by_cargo[type] += previous.delivered_cargo[type] - seen_cargo[type]; seen_cargo[type] = 0;
			}
			seen_income = 0; seen_expenses = 0; seen_delivered = 0;
			quarter = current_quarter;
		}
		income += company->cur_economy.income - seen_income;
		expenses += company->cur_economy.expenses - seen_expenses;
		delivered += company->cur_economy.delivered_cargo.GetSum<uint64_t>() - seen_delivered;
		// Native per-cargo deltas retain useful delivery provenance independently of income.
		for (uint type = 0; type < NUM_CARGO; ++type) {
			delivered_by_cargo[type] += company->cur_economy.delivered_cargo[type] - seen_cargo[type];
			seen_cargo[type] = company->cur_economy.delivered_cargo[type];
		}
		seen_income = company->cur_economy.income; seen_expenses = company->cur_economy.expenses;
		seen_delivered = company->cur_economy.delivered_cargo.GetSum<uint64_t>();
		const auto current_year = TimerGameEconomy::year.base();
		// Reconcile every expense category, including a year boundary, so paid work cannot disappear in a cash total.
		for (uint index = 0; index < to_underlying(ExpensesType::End); ++index) {
			const auto category = static_cast<ExpensesType>(index);
			if (current_year != year) {
				REQUIRE(current_year == year + 1);
				ledger[category] += company->yearly_expenses[1][category] - seen_ledger[category];
				seen_ledger[category] = 0;
			}
			ledger[category] += company->yearly_expenses[0][category] - seen_ledger[category];
			seen_ledger[category] = company->yearly_expenses[0][category];
		}
		year = current_year;
	}
};


#endif
