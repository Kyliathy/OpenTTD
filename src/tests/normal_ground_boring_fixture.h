/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_boring_fixture.h Real complete application frames for normally acquired paid boring and haulage.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6CDCB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_BORING_FIXTURE_H
#define TESTS_NORMAL_GROUND_BORING_FIXTURE_H

#include "normal_ground_runtime_fixture.h"
#include "normal_ground_video_fixture.h"

/** @param receiver Actual paid native disposal participant.
 * @return The real receiver's sole accepted stock;read-only and never a synthetic disposal. */
inline uint16_t ReadNormalGroundReceiverStock(const SpoilReceiver &receiver)
{
	const auto *industry = Industry::GetIfValid(receiver.industry); REQUIRE(industry != nullptr);
	const auto stock = industry->GetCargoAccepted(GetCargoTypeByLabel(GetSpoilReceiverRecipe(receiver.kind).cargo));
	REQUIRE(stock != std::end(industry->accepted));
	return stock->waiting;
}

/** Observe actual packets and physical wagon/set identities; never create cargo or move a participant.
 * @param tick Actual complete-frame tick used to bind packet/custody observations.
 */
inline void PrintNormalGroundBoreCargo(uint64_t tick)
{
	std::map<VehicleID, TBMWagonSetID> carried_sets;
	// Bind real wagon incarnations before inspecting the same physical stock;
	// a detached set record cannot stand in for an actually held cargo packet.
	for (const auto &set : CaptureTBMWagonSets()) {
		fmt::print("NORMAL_GROUND_BORE_SET tick={} set={} project={} state={} visited={} revision={} wagons={}\n",
			tick, set.id.base(), set.project.base(), to_underlying(set.state), set.visited_origin ? 1 : 0, set.revision, set.wagons.size());
		for (const auto id : set.wagons) carried_sets.emplace(id, set.id);
	}
	const auto earth = GetCargoTypeByLabel(GetSpoilReceiverRecipe(SpoilReceiverKind::Earth).cargo);
	const auto rock = GetCargoTypeByLabel(GetSpoilReceiverRecipe(SpoilReceiverKind::Rock).cargo);
	// Inspect only native spoil-capable wagons, retaining each actual packet's
	// quantity and provenance across face/bay exchanges and depot returns.
	for (const auto *wagon : Train::Iterate()) {
		if (wagon->cargo_cap == 0 || (wagon->cargo_type != earth && wagon->cargo_type != rock)) continue;
		const auto set = carried_sets.find(wagon->index);
		fmt::print("NORMAL_GROUND_BORE_WAGON tick={} set={} wagon={} front={} tile={} cargo={} amount={} capacity={}\n",
			tick, set == carried_sets.end() ? UINT32_MAX : set->second.base(), wagon->index.base(), wagon->First()->index.base(), wagon->tile.base(), wagon->cargo_type, wagon->cargo.StoredCount(), wagon->cargo_cap);
		for (const CargoPacket *packet : *wagon->cargo.Packets()) fmt::print("NORMAL_GROUND_BORE_PACKET tick={} wagon={} packet={} amount={} age={} feeder={} source={}\n",
			tick, wagon->index.base(), packet->index.base(), packet->Count(), packet->GetPeriodsInTransit(), packet->GetFeederShare().base(), packet->GetSourceTile().base());
	}
}

/** Full native GameLoop owns cuts,travel,exchanges,receiver intake and all company accounting.
 * @param expected_model Actual Standard/Deep design selected by the native isolated case.
 * @param scenario Independent output home for this normally acquired original work.
 */
inline void RunNormalGroundBoring(uint16_t expected_model = 0, std::string_view scenario = "normal-ground-boring")
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	const char *budget_text = std::getenv("OTTD_NORMAL_GROUND_TICKS"); REQUIRE(budget_text != nullptr);
	const std::string_view argument{budget_text}; uint32_t budget = 0;
	const auto parsed = std::from_chars(argument.data(), argument.data() + argument.size(), budget);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == argument.data() + argument.size());
	REQUIRE(budget > 0); REQUIRE(budget <= 65536);
	bool started = false, requested = false, finished = false;
	/** @param name Explicit retained return/resume flag from the selected predecessor. @return Validated native continuation flag. */
	const auto inherited_control = [](const char *name) {
		const char *value = std::getenv(name); REQUIRE(value != nullptr);
		REQUIRE((std::string_view{value} == "0" || std::string_view{value} == "1"));
		return std::string_view{value} == "1";
	};
	bool returned = inherited_control("OTTD_NORMAL_GROUND_RETURNED");
	bool resumed = inherited_control("OTTD_NORMAL_GROUND_RESUMED");
	REQUIRE((!resumed || returned));
	std::exception_ptr failure;
	CompanyID owner = CompanyID::Invalid(); TBMProjectID project_id{UINT32_MAX};
	Money cash = 0, loan = 0; uint64_t first_tick = 0;
	uint32_t first_cursor = 0; uint64_t first_earth = 0, first_rock = 0;
	uint64_t prior_earth = 0, prior_rock = 0;
	std::set<VehicleID> retiring_haulers;
	std::unique_ptr<NormalFreightEarnings> accounting;
	std::optional<std::tuple<uint8_t, uint8_t, uint32_t, uint8_t, uint8_t, uint32_t, uint64_t, uint64_t>> previous;
	std::set<TileIndex> hauler_visits;
	uint64_t max_hauler_load = 0; uint64_t last_trace_tick = 0;
	std::function<void()> begin = [&]() {
		try {
			REQUIRE(_game_mode == GameMode::Normal); REQUIRE_FALSE(_networking);
			REQUIRE(Company::IsValidID(_local_company)); owner = _local_company;
			AutoRestoreBackup actor{_current_company, owner};
			const auto projects = CaptureTBMProjects(); REQUIRE(projects.size() == 1);
			const auto &project = projects.front(); REQUIRE(project.company == owner); REQUIRE(project.model_id == expected_model);
			project_id = project.id;
			const auto *job = GetExcavationJob(project.job); REQUIRE(job != nullptr);
			first_cursor = job->cursor; first_earth = job->completed_earth_t; first_rock = job->completed_rock_t;
			// Completed earlier paid jobs remain native history. Their actual
			// receiver intake is a predecessor,not output from this new job.
			for (const auto &earlier : GetGeologyWorldState().jobs) if (earlier.id != job->id && earlier.company == owner) {
				REQUIRE(earlier.state == ExcavationJobState::Completed);
				prior_earth += earlier.completed_earth_t; prior_rock += earlier.completed_rock_t;
			}
			const auto *company = Company::Get(owner); cash = company->money; loan = company->current_loan;
			accounting = std::make_unique<NormalFreightEarnings>(*company);
			first_tick = TimerGameTick::counter;
			if (_pause_mode.Test(PauseMode::Normal)) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
			if (project.blocked == TBMBlockedReason::Paused) PayNormalFreightCommand<Commands::ControlTBMProject>("resume_boring", project.id, TBMProjectAction::Resume);
			const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
			// Retired paid haulers must vacate the shared line through actual depot
			// travel; do not move unrelated services or the assigned active hauler.
			for (auto *front : Train::Iterate()) {
				if (!front->IsFrontEngine() || front->owner != owner || front->index == assignments.front().hauler ||
					front->engine_type != GetUndergroundEngine(4) || GetTBMExchangeAssignmentForHauler(front->index) != nullptr) continue;
				uint64_t held = 0; for (const auto *part = front; part != nullptr; part = part->Next()) held += part->cargo.StoredCount();
				REQUIRE(held == 0); retiring_haulers.insert(front->index);
				// A retired hauler can stand on the shared Surface line.
				// A real native depot visit vacates it;never move the train privately.
				if (!front->IsStoppedInDepot()) {
					if (!front->current_order.IsType(OT_GOTO_DEPOT)) PayNormalFreightCommand<Commands::SendVehicleToDepot>("store_retired_hauler", front->index, DepotCommandFlags{}, VehicleListIdentifier{});
					if (front->vehstatus.Test(VehState::Stopped)) PayNormalFreightCommand<Commands::StartStopVehicle>("start_retired_hauler", front->tile, front->index, true);
				}
			}
			// Start only the two original assigned fronts through the player command; retain every other service.
			for (const auto id : {project.cutter, assignments.front().hauler}) {
				auto *front = Train::GetIfValid(id); REQUIRE(front != nullptr);
				if (front->vehstatus.Test(VehState::Stopped)) PayNormalFreightCommand<Commands::StartStopVehicle>("start_boring_participant", front->tile, front->index, true);
			}
			PrintNormalGroundBoreCargo(first_tick); started = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &begin};
	IConsole::CmdRegister("normal_ground_boring", ObserveNormalGroundNewgame);
	/** Read complete native ownership between simulation frames;only the saver issues a pause. */
	const auto observe = [&]() {
		accounting->Observe();
		const auto *project = GetTBMProject(project_id); REQUIRE(project != nullptr);
		const auto *job = GetExcavationJob(project->job); REQUIRE(job != nullptr);
		const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
		const auto &assignment = assignments.front(); const auto *hauler = Train::GetIfValid(assignment.hauler); REQUIRE(hauler != nullptr);
		hauler_visits.insert(hauler->tile);
		uint64_t held = 0; for (const auto *part = hauler; part != nullptr; part = part->Next()) held += part->cargo.StoredCount();
		max_hauler_load = std::max(max_hauler_load, held);
		uint64_t earth = 0, rock = 0, earth_waiting = 0, rock_waiting = 0;
		bool processing_settled = true;
		// Reconcile each real receiver with the same physical spoil history; no synthetic disposal is credited.
		for (const auto &receiver : CaptureSpoilReceivers()) {
			const auto waiting = ReadNormalGroundReceiverStock(receiver);
			processing_settled &= waiting < GetSpoilReceiverRecipe(receiver.kind).batch_t;
			if (receiver.kind == SpoilReceiverKind::Earth) { earth += receiver.disposed_t; earth_waiting += waiting; }
			if (receiver.kind == SpoilReceiverKind::Rock) { rock += receiver.disposed_t + receiver.crushed_t; rock_waiting += waiting; }
		}
		const auto state = std::tuple{to_underlying(project->phase), to_underlying(project->blocked), job->cursor,
			to_underlying(assignment.phase), to_underlying(assignment.wait), assignment.exchanges, earth, rock};
		// Retain transitions and a bounded heartbeat so waits and recovery are observable without per-frame log growth.
		if (!previous || state != *previous || TimerGameTick::counter - last_trace_tick >= 512) {
			fmt::print("NORMAL_GROUND_BORE_TRACE tick={} project={} phase={} blocked={} cursor={} routes={} hauler={} hauler_tile={} hauler_phase={} hauler_wait={} exchanges={} held={} earth_received={} rock_received={} earth_waiting={} rock_waiting={} cash={} release_error={}\n",
				TimerGameTick::counter, project_id.base(), to_underlying(project->phase), to_underlying(project->blocked), job->cursor, job->route.size(),
				assignment.hauler.base(), hauler->tile.base(), to_underlying(assignment.phase), to_underlying(assignment.wait), assignment.exchanges, held,
				earth + earth_waiting, rock + rock_waiting, earth_waiting, rock_waiting, Company::Get(owner)->money.base(), GetTBMProjectReleaseProblem(project_id).base());
			PrintNormalGroundBoreCargo(TimerGameTick::counter); std::fflush(stdout);
			previous = state; last_trace_tick = TimerGameTick::counter;
		}
		// Finish only after actual return/resume and all native spoil custody settles, or preserve a bounded unfinished successor.
		if ((returned && resumed && job->cursor == job->route.size() && IsTBMProjectFinished(project->phase) &&
			GetTBMProjectReleaseProblem(project_id) == INVALID_STRING_ID && processing_settled && earth + earth_waiting == prior_earth + job->completed_earth_t && rock + rock_waiting == prior_rock + job->completed_rock_t) ||
			TimerGameTick::counter - first_tick >= budget) requested = true;
	};
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&](uint) {
		// Simulation observation belongs only to an active successful segment; exit/recovery never consumes an extra frame.
		if (!started || requested || failure) return;
		try { observe(); } catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	// These are ordinary player actions between complete native frames. A
	// partial return and a settled resume exercise a real second haulage cycle
	// even though this small assembly room fits within one hopper capacity.
	const IntervalTimer<TimerGameRealtime> controls({std::chrono::milliseconds(1), TimerGameRealtime::Trigger::Always}, [&](auto) {
		// Player controls stop at the selected segment boundary; no later command may spend on a closed segment.
		if (!started || requested || finished || failure) return;
		try {
			const auto *project = GetTBMProject(project_id); REQUIRE(project != nullptr);
			const auto *job = GetExcavationJob(project->job); REQUIRE(job != nullptr);
			AutoRestoreBackup actor{_current_company, owner};
			// A real partial cut triggers public return-and-park; the settled successor can then resume the same paid project.
			if (!returned && job->cursor >= 3 && (job->completed_earth_t != 0 || job->completed_rock_t != 0) && !IsTBMProjectFinished(project->phase)) {
				PayNormalFreightCommand<Commands::ControlTBMProject>("partial_return_and_park", project_id, TBMProjectAction::ReturnAndPark);
				returned = true;
			} else if (returned && !resumed && project->phase == TBMPhase::Parked && GetTBMProjectReleaseProblem(project_id) == INVALID_STRING_ID) {
				PayNormalFreightCommand<Commands::ControlTBMProject>("settled_resume", project_id, TBMProjectAction::Resume);
				resumed = true;
			}
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	const IntervalTimer<TimerGameRealtime> saver({std::chrono::milliseconds(1), TimerGameRealtime::Trigger::Always}, [&](auto) {
		// Save only one completed observation boundary and never serialize a partial/failing command.
		if (!started || !requested || finished || failure) return;
		try {
			observe(); AutoRestoreBackup actor{_current_company, owner};
			REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
			const auto *project = GetTBMProject(project_id); REQUIRE(project != nullptr);
			const auto *job = GetExcavationJob(project->job); REQUIRE(job != nullptr);
			const auto *company = Company::Get(owner); CHECK(company->current_loan == loan);
			Money total = 0;
			// Every expense category contributes to the original cash reconciliation;
			// work costs cannot be concealed by unrelated freight income.
			for (uint index = 0; index < to_underlying(ExpensesType::End); ++index) {
				const auto cost = accounting->ledger[static_cast<ExpensesType>(index)]; total += cost;
				fmt::print("NORMAL_GROUND_BORE_LEDGER category={} cost={}\n", index, cost.base());
			}
			CHECK(company->money == cash - total);
			CHECK(job->cursor >= first_cursor); CHECK(job->completed_earth_t >= first_earth); CHECK(job->completed_rock_t >= first_rock);
			uint64_t earth = 0, rock = 0, earth_waiting = 0, rock_waiting = 0, fees = 0;
			bool processing_settled = true;
			// Reconcile each real receiver with the same physical spoil history; no synthetic disposal is credited.
		for (const auto &receiver : CaptureSpoilReceivers()) {
				const auto waiting = ReadNormalGroundReceiverStock(receiver);
				const auto batch = GetSpoilReceiverRecipe(receiver.kind).batch_t;
				processing_settled &= waiting < batch;
				fmt::print("NORMAL_GROUND_BORE_RECEIVER industry={} kind={} disposed={} crushed={} waiting={} batch={} aggregate={} aggregate_made={} residue={} fees={}\n",
					receiver.industry.base(), to_underlying(receiver.kind), receiver.disposed_t, receiver.crushed_t, waiting, batch, receiver.aggregate, receiver.aggregate_t, receiver.residue_t, receiver.fees);
				if (receiver.kind == SpoilReceiverKind::Earth) { earth += receiver.disposed_t; earth_waiting += waiting; }
				if (receiver.kind == SpoilReceiverKind::Rock) { rock += receiver.disposed_t + receiver.crushed_t; rock_waiting += waiting; }
				fees += receiver.fees;
			}
			const bool complete = returned && resumed && job->cursor == job->route.size() && IsTBMProjectFinished(project->phase) &&
				GetTBMProjectReleaseProblem(project_id) == INVALID_STRING_ID && processing_settled && earth + earth_waiting == prior_earth + job->completed_earth_t && rock + rock_waiting == prior_rock + job->completed_rock_t;
			if (complete) { CHECK(earth + earth_waiting == prior_earth + job->completed_earth_t); CHECK(rock + rock_waiting == prior_rock + job->completed_rock_t); }
			const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
			uint stored = 0;
			// Each retired participant must really reach storage before a completed
			// segment is accepted; a stopped flag alone does not vacate shared track.
			for (const auto id : retiring_haulers) {
				const auto *front = Train::GetIfValid(id); REQUIRE(front != nullptr);
				const bool in_depot = front->IsStoppedInDepot(); stored += in_depot ? 1 : 0;
				fmt::print("NORMAL_GROUND_BORE_PARKED vehicle={} tile={} stored={}\n", id.base(), front->tile.base(), in_depot ? 1 : 0);
				if (complete) CHECK(in_depot);
			}
			const auto counts = CountNormalGroundRockStates();
			const auto paths = GetTestCaseArtifactPaths("acceptance", scenario);
			for (const auto name : {"boring-progress.sav", "boring-progress-repeat.sav"})
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
			fmt::print("NORMAL_GROUND_BORED project={} job={} cutter={} hauler={} model={} first_tick={} tick={} first_cursor={} cursor={} routes={} first_earth={} first_rock={} earth={} rock={} work_cost={} prior_earth={} prior_rock={} earth_received={} rock_received={} earth_processed={} rock_processed={} earth_waiting={} rock_waiting={} fees={} opening_cash={} cash={} loan={} complete={} returned={} resumed={} phase={} blocked={} exchanges={} visited={} peak_load={} solid={} excavating={} excavated={} reserved={} release_error={} stored_haulers={}\n",
				project_id.base(), project->job.base(), project->cutter.base(), assignments.front().hauler.base(), expected_model, first_tick, TimerGameTick::counter, first_cursor, job->cursor, job->route.size(), first_earth, first_rock,
				job->completed_earth_t, job->completed_rock_t, job->completed_work_cost.base(), prior_earth, prior_rock, earth + earth_waiting - prior_earth, rock + rock_waiting - prior_rock, earth, rock, earth_waiting, rock_waiting, fees, cash.base(), company->money.base(), loan.base(), complete ? 1 : 0, returned ? 1 : 0, resumed ? 1 : 0,
				to_underlying(project->phase), to_underlying(project->blocked), assignments.front().exchanges, hauler_visits.size(), max_hauler_load,
				counts[0], counts[1], counts[2], counts[3], GetTBMProjectReleaseProblem(project_id).base(), stored);
			std::fflush(stdout); finished = true; _exit_game = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	const auto driver = std::getenv("OTTD_NORMAL_GROUND_CAPTURE") != nullptr ? std::string{"ground-acceptance-null"} : fmt::format("null:ticks={}", budget + 3000);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(),
		"-v", driver, "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null", "-I", "original_dos", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0);
	if (failure) std::rethrow_exception(failure);
	REQUIRE(started); REQUIRE(finished);
}

#endif /* TESTS_NORMAL_GROUND_BORING_FIXTURE_H */
