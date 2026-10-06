/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_economics.cpp Whole paid native excavation projects with physical haulage and disposal.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 20
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md, checkpoint 7.8
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_workflow_fixture.h"
#include "../economy_func.h"

#include "../safeguards.h"

/** Every paid setup/cut/operating/disposal expense balances against actual cash on both normal and harder Deep projects. */
TEST_CASE("Complete native standard and Deep projects retain their full paid cost ledger", "[tbm-economics]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const MapLayer layer = GENERATE(MapLayer::Underground, MapLayer::DeepUnderground);
	AutoRestoreBackup prices(_price);
	AutoRestoreBackup economy(_economy);
	AutoRestoreBackup vehicle_costs(_settings_game.difficulty.vehicle_costs, uint8_t{1});
	AutoRestoreBackup construction_cost(_settings_game.difficulty.construction_cost, uint8_t{1});
	_economy.inflation_prices = _economy.inflation_payment = 1 << 16; // Declared medium-difficulty, base-currency comparison; existing canonical GRF price modifiers stay intact.
	UndergroundWorkflowWorld world(layer, true);
	Company *company = Company::Get(_current_company);
	const Money initial = 10000000;
	REQUIRE(initial - company->money == world.setup_cost);
	REQUIRE(world.setup_cost > 0);
	const auto setup_expenses = company->yearly_expenses[0];
	const auto project_id = RunUndergroundExcavationWorkflow(world, Ticks::DAY_TICKS);
	const TBMProject project = *GetTBMProject(project_id);
	const auto *job = GetExcavationJob(project.job);
	REQUIRE(job != nullptr);
	REQUIRE(job->state == ExcavationJobState::Completed);
	REQUIRE(GetTBMExchangeAssignmentForHauler(world.hauler->index)->exchanges > 1);

	// Flush only genuine accrued balances through their existing owners; the last partial day must be in the project invoice.
	ChargeTBMProjectCosts(project_id);
	world.hauler->OnNewEconomyDay();
	const auto &expenses = company->yearly_expenses[0];
	TBMProject durations = project; // Read-only quote of all observed durations, independent of daily bucket resets.
	durations.active_ticks = world.measured_active_ticks;
	durations.idle_ticks = world.measured_idle_ticks;
	durations.cost_remainder = 0;
	const Money tbm_operating = ComputeTBMRecurringCharge(durations).cost;
	durations.idle_ticks = 0;
	const Money active_operating = ComputeTBMRecurringCharge(durations).cost;
	const Money idle_operating = tbm_operating - active_operating; // Allocate the owner's combined fractional carry to the idle component; their exact sum remains the paid invoice.
	const Money haulage = expenses[ExpensesType::TrainRun] - tbm_operating;
	const Money purchase = expenses[ExpensesType::NewVehicles] - setup_expenses[ExpensesType::NewVehicles];
	const Money disposal = expenses[ExpensesType::Other] - setup_expenses[ExpensesType::Other];
	CHECK(expenses[ExpensesType::Construction] - setup_expenses[ExpensesType::Construction] == job->completed_work_cost);
	CHECK(disposal > 0);
	CHECK(static_cast<uint64_t>(disposal.base()) == GetSpoilReceiver(world.earth->index)->fees + GetSpoilReceiver(world.stone->index)->fees);
	CHECK(expenses[ExpensesType::TrainRevenue] == 0); // Spoil delivery cannot masquerade as transport income.
	CHECK(tbm_operating > 0);
	CHECK(haulage > 0);
	CHECK(purchase > 0);
	CHECK(world.measured_active_ticks > 0);
	CHECK(world.measured_idle_ticks > 0);
	const Money total = world.setup_cost + purchase + job->completed_work_cost + active_operating + idle_operating + haulage + disposal;
	CHECK(initial - company->money == total);
	WARN("MLVS_TBM_PROJECT layer=" << uint{to_underlying(layer)} << " seed=420 date=1980-01-01 startup_hold_ticks=" << Ticks::DAY_TICKS << " ticks=" << world.ticks
		<< " cuts=" << job->cursor << " exchanges=" << GetTBMExchangeAssignmentForHauler(world.hauler->index)->exchanges
		<< " setup=" << world.setup_cost.base() << " purchase=" << purchase.base() << " work_support=" << job->completed_work_cost.base()
		<< " active_ticks=" << world.measured_active_ticks << " idle_ticks=" << world.measured_idle_ticks
		<< " active_cost=" << active_operating.base() << " idle_cost=" << idle_operating.base()
		<< " haulage=" << haulage.base() << " disposal=" << disposal.base() << " total=" << total.base()
		<< " earth_t=" << job->completed_earth_t << " rock_t=" << job->completed_rock_t);
}
