/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_depot_trace.cpp Actual recurring-depot custody, packet age, receiver and invoice trace with deterministic replay.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Groups 13B.3/13B.4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md (retained producer trace)
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "underground_depot_fixture.h"
#include "tbm_exchange_fixture.h"
#include <map>

#include "../safeguards.h"

/** Observe physical ownership at every frame and retain complete normalized state at each logistics/work transition. */
class DepotLogisticsTrace {
	std::map<VehicleID, uint> vehicles; ///< Observer-only ordinal labels; exact real IDs are also printed in each run.
	std::map<CargoPacketID, uint> packets; ///< Observer-only packet labels; no cargo is allocated or moved here.
	std::string previous; ///< Previous public phase/cursor/wait key, so unchanged frames do not bloat the retained trace.
	TBMPacketCustody custody; ///< Existing owner-list audit, including every vehicle and station packet reference.
	uint start; ///< Declared scenario-relative physical-frame origin after ordinary preparation/assignment.
public:
	std::vector<std::string> rows; ///< Full canonical transition rows compared across real native replays.

	/** @param workflow Actual paid prepared world; the observer captures no replacement gameplay state. */
	explicit DepotLogisticsTrace(const UndergroundDepotWorkflow &workflow) : start(workflow.world.ticks) {}

	/** @param id Actual vehicle identity. @return Stable first-observed replay label; the real object remains untouched. */
	uint VehicleLabel(VehicleID id) { return this->vehicles.try_emplace(id, static_cast<uint>(this->vehicles.size())).first->second; }

	/** @param id Actual packet identity. @return Stable first-observed replay label; no packet registry is replaced. */
	uint PacketLabel(CargoPacketID id) { return this->packets.try_emplace(id, static_cast<uint>(this->packets.size())).first->second; }

	/**
	 * Check conservation/sole custody, then record public state at an actual transition.
	 * @param workflow Actual world driven by its normal tick and owning commands.
	 * @param id Actual further-project identity, never a synthetic job reference.
	 * @param force True records the final invoice flush even when phases have not changed.
	 */
	void Observe(const UndergroundDepotWorkflow &workflow, TBMProjectID id, bool force = false)
	{
		const auto *project = GetTBMProject(id);
		REQUIRE(project != nullptr);
		const auto *job = GetExcavationJob(project->job);
		REQUIRE(job != nullptr);
		const auto *assignment = GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index);
		REQUIRE(assignment != nullptr);
		REQUIRE(this->custody.IsSole());
		std::array<uint64_t, 2> held{};
		// Every actual vehicle contributes once to real spoil custody, regardless of which original set now rides the cutter or hauler.
		for (const Vehicle *vehicle : Vehicle::Iterate()) {
			const auto kind = GetSpoilReceiverKindForCargo(vehicle->cargo_type);
			if (kind != SpoilReceiverKind::End) held[kind == SpoilReceiverKind::Earth ? 0 : 1] += vehicle->cargo.TotalCount();
		}
		// Actual platform inventory is another physical owner; catchment proximity never creates a second copy in a receiver or train.
		for (const Station *station : Station::Iterate()) {
			for (const auto [label, index] : {std::pair{CargoLabel{"ERTH"}, size_t{0}}, {CargoLabel{"ROCK"}, size_t{1}}}) {
				const auto &goods = station->goods[GetCargoTypeByLabel(label)];
				if (goods.HasData()) held[index] += goods.GetData().cargo.TotalCount();
			}
		}
		const auto *earth = GetSpoilReceiver(workflow.world.earth->index);
		const auto *rock = GetSpoilReceiver(workflow.world.stone->index);
		CHECK(held[0] + earth->disposed_t + workflow.world.earth->accepted.front().waiting == workflow.first_earth + job->completed_earth_t);
		CHECK(held[1] + rock->disposed_t + rock->crushed_t + workflow.world.stone->accepted.front().waiting == workflow.first_rock + job->completed_rock_t);
		const Company *company = Company::Get(project->company);
		Money invoiced = 0;
		// Native expense categories are the single paid-money ledger; pending operating buckets are recorded separately and cannot masquerade as paid cash.
		for (const auto expense : company->yearly_expenses[0]) invoiced += expense;
		CHECK(Money{10000000} - company->money == invoiced);
		const std::string key = fmt::format("{}/{}/{}/{}/{}/{}", to_underlying(project->phase), to_underlying(project->blocked), job->cursor,
			to_underlying(assignment->phase), to_underlying(assignment->wait), assignment->exchanges);
		// Custody/conservation is checked every frame; retain full descriptive rows only at real work/logistics transitions or final billing.
		if (!force && !this->rows.empty() && this->previous == key) return;
		this->previous = key;
		std::string row = fmt::format("frame={} state={} work={}/{}/{} buckets={}/{}/{} cash={}/{} first={}/{}/{} receiver={}/{}/{}/{}/{}/{}/{}/{}",
			workflow.world.ticks - this->start, key, job->completed_earth_t, job->completed_rock_t, job->completed_work_cost.base(),
			project->active_ticks, project->idle_ticks, project->cost_remainder, company->money.base(), company->money_fraction,
			workflow.first_earth, workflow.first_rock, workflow.first_work_cost.base(), earth->disposed_t, earth->interval_ticks, earth->interval_intake,
			rock->disposed_t, rock->crushed_t, rock->interval_ticks, rock->interval_intake, earth->fees + rock->fees);
		// The authoritative owner digests cover the complete geological job, project/support, exchange queue/set and receiver records.
		// These declared fresh worlds allocate identical native IDs; packet/unit ordinals below additionally expose physical custody.
		row += fmt::format(" owners={}/{}/{}/{}", ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeTBMProjectDigest(),
			ComputeTBMExchangeDigest(), ComputeSpoilReceiverDigest());
		std::string physical;
		// Keep whole-part positions, carriage identities, ordered packets, age, provenance and feeder credit together;
		// exchange may move the unit but may never rebuild its load or lose an articulated part.
		for (const Train *head : {Train::Get(project->cutter), workflow.world.hauler}) {
			row += " train[";
			for (const Train *part = head; part != nullptr; part = part->Next()) {
				row += fmt::format(" V{}@{}:{},{},{}:{}:{}:{}", this->VehicleLabel(part->index), part->tile.base(), part->x_pos, part->y_pos, part->z_pos,
					part->track.base(), to_underlying(part->direction), part->cargo.TotalCount());
				physical += fmt::format(" V{}={}", this->VehicleLabel(part->index), part->index.base());
				for (const CargoPacket *packet : *part->cargo.Packets()) {
					row += fmt::format(" P{}:{}/{}/{}/{}/{}/{}", this->PacketLabel(packet->index), packet->Count(), packet->GetPeriodsInTransit(),
						packet->GetSource().id, to_underlying(packet->GetSource().type), packet->GetFirstStation().base(), packet->GetFeederShare().base());
					physical += fmt::format(" P{}={}", this->PacketLabel(packet->index), packet->index.base());
				}
			}
			row += ']';
		}
		const auto *set = GetTBMWagonSet(assignment->set);
		REQUIRE(set != nullptr);
		row += fmt::format(" set={}/{} units=", to_underlying(set->state), set->visited_origin);
		for (VehicleID unit : set->wagons) row += fmt::format("{},", this->VehicleLabel(unit)); // Ordered carried units expose custody changes at the same native exchange boundary.
		row += " expenses=";
		for (const auto expense : company->yearly_expenses[0]) row += fmt::format("{},", expense.base()); // Retain every actual native invoice category, including zero-income spoil transport.
		this->rows.push_back(row);
		fmt::print("DEPOT_TRACE {} raw_ids={}\n", row, physical);
	}
};

/**
 * Run one complete real paid clearing/recommissioning/depot/disposal scenario and retain its physical ledger.
 * @return Canonical transition rows with identity ordinals for comparing another declared native replay.
 */
static std::vector<std::string> RunRecordedDepotLoop()
{
	UndergroundDepotWorkflow workflow;
	workflow.Prepare();
	const TBMProjectID id = workflow.StartFurther();
	DepotLogisticsTrace trace(workflow);
	trace.Observe(workflow, id);
	/** @return Whether the further paid job and its final mandatory physical Surface disposal have both finished. */
	const auto delivered = [&]() {
		const auto *assignment = GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index);
		return IsTBMProjectFinished(GetTBMProject(id)->phase) && assignment->phase == TBMHaulerPhase::ReturnEmpties;
	};
	/** Read-only post-frame callback; every movement/cut/coupling/disposal still belongs to the same native owner. */
	const auto observe = [&]() { trace.Observe(workflow, id); };
	workflow.DriveUntil(delivered, 50000, observe);
	REQUIRE(GetTBMExchangeAssignmentForHauler(workflow.world.hauler->index)->exchanges >= 2);
	ChargeTBMProjectCosts(id);
	workflow.world.hauler->OnNewEconomyDay();
	trace.Observe(workflow, id, true);
	fmt::print("DEPOT_TRACE_COMPLETE rows={} command_cost={} first_cost={} final_cash={}\n", trace.rows.size(), workflow.commands.base(),
		workflow.first_project_cost.base(), Company::Get(_current_company)->money.base());
	return trace.rows;
}

/** Two native worlds replay the same command/frame schedule with exact normalized physical/cargo/receiver/invoice transitions. */
TEST_CASE("Recurring underground-depot traces replay every packet and native invoice", "[tbm-depot-replay]")
{
	MockEnvironment::Instance();
	LoadGeneratedWindowTestLanguage();
	const auto first = RunRecordedDepotLoop();
	const auto second = RunRecordedDepotLoop();
	REQUIRE_FALSE(first.empty());
	CHECK(first == second);
}
