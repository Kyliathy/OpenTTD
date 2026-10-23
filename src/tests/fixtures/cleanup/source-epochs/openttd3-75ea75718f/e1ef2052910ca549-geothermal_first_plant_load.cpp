/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_load.cpp Ordinary application loading of the original paid first-plant world; no replacement fixture is constructed.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FA.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-e-first-plant-bootstrap.md.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_first_plant_fixture.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../openttd.h"
#include "../misc_cmd.h"
#include "../tbm_replanning.h"
#include "../timer/timer.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"
#include "../pathfinder/follow_track.hpp"
#include "../pathfinder/yapf/yapf.h"
#include <set>

static std::function<void()> *_first_plant_file_observer = nullptr; ///< Process-local observation after actual native afterload, never a saved/gameplay owner.

/**
 * Describe the actual retained native route when a loaded hauler reports lost.
 * @param project Original live paid project.
 * @param assignment Its actual native haulage cycle, never a replacement order.
 */
static void TraceLoadedFirstPlantRoute(const TBMProject &project, const TBMExchangeAssignment &assignment)
{
	// Long face waits and genuinely lost travel need the same physical trace;
	// ordinary short dwell/receiver intervals do not need a graph diagnostic.
	if (assignment.wait != TBMHaulerWait::BlockedLine && project.phase_ticks < 5000) return;
	const auto *hauler = Train::Get(assignment.hauler);
	fmt::print("FIRST_PLANT_BLOCKED hauler={}\ncutter={}\n", DescribeTrain(hauler), DescribeTrain(Train::Get(project.cutter)));
	const auto goal = GetTBMHaulerSteeringTarget(*hauler);
	const auto stand = FindTBMFaceStand(project);
	fmt::print("FIRST_PLANT_FACE kind={} stand={} goal={} dirs={} held={} proposal={}\n", to_underlying(GetTBMMeetingKind(project)),
		stand ? stand->tile.base() : UINT32_MAX, goal ? goal->tile.base() : UINT32_MAX, goal ? goal->trackdirs.base() : 0,
		IsTBMHaulerHeld(*hauler), to_underlying(DescribeTBMFaceTransfer({project.id, assignment.hauler}).error));
	// Read the same native follower as routing; the diagnostic neither lays a
	// missing piece nor authorizes an otherwise closed shaft or support cell.
	/** @param start Actual moving end. @param direction Its directed departure. @return Whether the native admitted graph reaches the original station. */
	const auto reaches_origin = [&](TileIndex start, Trackdir direction) {
		std::vector<std::pair<TileIndex, Trackdir>> pending{{start, direction}};
		std::set<std::pair<TileIndex, Trackdir>> visited;
		uint failures = 0; bool origin = false, target = false;
		for (size_t index = 0; index < pending.size() && visited.size() < 5000; ++index) { // Bound a diagnostic independently of game work; every directed rail state is examined at most once.
			const auto [tile, trackdir] = pending[index];
			if (!visited.emplace(tile, trackdir).second) continue;
			if (IsRailStationTile(tile) && GetStationIndex(tile) == project.origin) origin = true; // Only the original actual station satisfies loaded travel.
			if (goal.has_value() && tile == goal->tile && goal->trackdirs.Test(trackdir)) target = true; // Exact native steering trackdirs decide arrival; a nearby safe cell is not its stand.
			CFollowTrackRail follower(hauler);
			if (!follower.Follow(tile, trackdir)) {
				if (failures++ < 24) fmt::print("FIRST_PLANT_ROUTE_END tile={} xy={}:{}:{} td={} next={} err={}\n", tile.base(),
					to_underlying(GetTileLayer(tile)), TileX(tile), TilePlaneY(tile), to_underlying(trackdir), follower.new_tile.base(), to_underlying(follower.err));
				continue;
			}
			// All native admitted continuations matter at a chamber junction; no
			// locally chosen shortcut stands in for the actual follower's answer.
			for (Trackdir next : follower.new_td_bits) pending.emplace_back(follower.new_tile, next);
		}
		fmt::print("FIRST_PLANT_ROUTE_REACHABLE states={} origin={} target={} forbid90={}\n", visited.size(), origin, target, _settings_game.pf.forbid_90_deg);
		return origin;
	};
	const bool forward = reaches_origin(hauler->GetMovingFront()->tile, hauler->GetMovingFront()->GetVehicleTrackdir());
	const bool reverse = reaches_origin(hauler->GetMovingBack()->tile, ReverseTrackdir(hauler->GetMovingBack()->GetVehicleTrackdir()));
	fmt::print("FIRST_PLANT_ROUTE_DIRECTION forward={} reverse={} native_reverse={}\n", forward, reverse, YapfTrainCheckReverse(hauler));
	// Both real endpoints must allow the loaded outbound role, irrespective
	// of the cutter's current face or the process-local observer's lifetime.
	for (TileIndex tile : {project.portal, GetLayerPortalPairTile(project.portal)}) {
		fmt::print("FIRST_PLANT_ROUTE_PORTAL tile={} dirs={} admitted={} busy={}\n", tile.base(),
			GetLayerPortalAdmittedTrackdirs(tile, *hauler, GetLayerPortalDoorDirection(tile)).base(),
			IsTBMHaulerAdmittedThroughPortal(*hauler, tile), IsLayerPortalMagazineBusy(tile, *hauler));
	}
}

/** @param path Fresh invocation-owned destination. Save actual native owners synchronously without changing their quantities or retained pause policy. */
static void SaveLoadedFirstPlant(const std::filesystem::path &path)
{
	AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/**
 * Quote the union of still-unearned native cuts and retain earned receipts.
 * @param project Original live paid Deep project.
 * @return Exact expected complete Earth/Rock/work-support totals, without changing terrain or reservations.
 */
static ExcavationCutResult QuoteLoadedFirstPlant(const TBMProject &project)
{
	const auto &job = *GetExcavationJob(project.job);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(project.company)->money;
	const auto random = std::array{_random.state[0], _random.state[1]};
	std::set<TileIndex> union_cells;
	// The canonical sweeps, including turning rims and access, define the
	// charged union. Previously earned open cells cannot be charged twice.
	for (const auto &cut : job.route) union_cells.insert(cut.swept_tiles.begin(), cut.swept_tiles.end());
	const std::vector<TileIndex> cells(union_cells.begin(), union_cells.end());
	auto quote = QuoteExcavationCells(cells, project.model_id, project.job);
	REQUIRE(quote.error == ExcavationError::None);
	quote.earth_t += job.completed_earth_t; quote.rock_t += job.completed_rock_t;
	quote.cost += job.completed_work_cost;
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(project.company)->money == cash); CHECK(std::array{_random.state[0], _random.state[1]} == random);
	fmt::print("FIRST_PLANT_QUOTE job={} cursor={}/{} remaining_cells={} earth={} rock={} work_support={}\n",
		project.job.base(), job.cursor, job.route.size(), quote.new_cells, quote.earth_t, quote.rock_t, quote.cost.base());
	return quote;
}

/**
 * Reconcile all native spoil custody against every retained paid job.
 * @param company Actual scenario company; unrelated owners cannot satisfy its receipts.
 */
static void CheckLoadedFirstPlantSpoil(CompanyID company)
{
	uint64_t earth = 0, rock = 0, earth_receipts = 0, rock_receipts = 0, fees = 0;
	// All original jobs remain geological history after project retirement;
	// Standard spoil and the current Deep work must balance together.
	for (const auto &job : GetGeologyWorldState().jobs) {
		if (job.company != company) continue;
		earth_receipts += job.completed_earth_t; rock_receipts += job.completed_rock_t;
	}
	// Every physical hopper part owns its actual packets. No exchange-set
	// scalar or station proximity is a substitute for this custody count.
	for (const auto *part : Train::Iterate()) {
		if (part->owner != company) continue;
		if (part->cargo_type == GetCargoTypeByLabel(CargoLabel{"ERTH"})) earth += part->cargo.TotalCount();
		if (part->cargo_type == GetCargoTypeByLabel(CargoLabel{"ROCK"})) rock += part->cargo.TotalCount();
	}
	// Receivers retain both lifetime processing and still-held native stock.
	// Their actual fees and old crusher history cannot be cleared on reload.
	for (const auto &receiver : CaptureSpoilReceivers()) {
		const auto *industry = Industry::Get(receiver.industry);
		if (industry->founder != company) continue;
		const uint64_t waiting = industry->accepted.front().waiting;
		fees += receiver.fees;
		if (receiver.kind == SpoilReceiverKind::Earth) earth += receiver.disposed_t + waiting;
		else rock += receiver.disposed_t + receiver.crushed_t + waiting;
		fmt::print("FIRST_PLANT_RECEIVER id={} kind={} waiting={} disposed={} crushed={} aggregate={} fees={}\n",
			receiver.industry.base(), to_underlying(receiver.kind), waiting, receiver.disposed_t, receiver.crushed_t, receiver.aggregate, receiver.fees);
	}
	CHECK(earth == earth_receipts); CHECK(rock == rock_receipts);
	fmt::print("FIRST_PLANT_SPOIL earth={} rock={} fees={} packets={}\n", earth, rock, fees, CargoPacket::GetNumItems());
}

/**
 * Verify completed actual native quantities and every declared plant-floor cell.
 * @param project Original paid Deep project, still retained before release.
 * @param expected Canonical union quote including its actual earned history.
 */
static void CheckLoadedFirstPlantComplete(const TBMProject &project, const ExcavationCutResult &expected)
{
	const auto &job = *GetExcavationJob(project.job);
	REQUIRE(job.cursor == job.route.size());
	CHECK(job.completed_earth_t == expected.earth_t); CHECK(job.completed_rock_t == expected.rock_t);
	CHECK(job.completed_work_cost == expected.cost);
	const auto layout = DescribeFirstPlantLayout(GetGeologyWorldState().vents.front());
	// Every building, service, utility and access target must come from the
	// same actual paid work, including all corners of the168-cell declaration.
	for (const auto tile : GeothermalFirstPlantRig::Rectangle(layout.deep_start, layout.deep_end)) CHECK(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
}

/**
 * Append the next declared first-plant column only when existing paid work is
 * nearly complete; actual saved requests determine progress across processes.
 * @param id Original live Deep project; its cutter, job, cargo and paid history stay native-owned.
 * @return Whether an actual accepted extension changed the expected union totals.
 */
static bool ExtendLoadedFirstPlant(TBMProjectID id)
{
	const auto *project = GetTBMProject(id); REQUIRE(project != nullptr);
	const auto *job = GetExcavationJob(project->job); REQUIRE(job != nullptr);
	// Only the last unfinished Deep cut accepts an extension. Geology can
	// finish before the physical controller has returned and settled, so its
	// completed cursor also ends extension requests during that real withdrawal.
	if (project->model_id != 1 || IsTBMProjectFinished(project->phase) || job->cursor >= job->route.size() || job->cursor + 1 < job->route.size()) return false;
	REQUIRE(job->cursor < job->route.size());
	const auto &vent = GetGeologyWorldState().vents.front();
	const uint first_column = static_cast<uint>(vent.x) + 4, last_column = static_cast<uint>(vent.x) + 10;
	const uint first_row = static_cast<uint>(vent.y) - 2, last_row = static_cast<uint>(vent.y) + 11;
	// These seven bounded columns complete the exact declared12x14 chamber.
	// Existing saved requested cells or genuinely earned swept rims need no
	// duplicate extension, even after the process-local observer is recreated.
	for (uint column = first_column; column <= last_column; ++column) {
		bool requested = true, open = true;
		for (uint row = first_row; row <= last_row; ++row) {
			const auto tile = TileXYL(column, row, MapLayer::DeepUnderground);
			requested &= std::ranges::binary_search(job->requested_tiles, tile);
			open &= GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated;
		}
		if (requested || open) continue; // Only a missing actual target column may enlarge the original accepted job.
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Pause).Succeeded());
		const auto start = TileXYL(column, first_row, MapLayer::DeepUnderground), end = TileXYL(column, last_row, MapLayer::DeepUnderground);
		const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState()); const auto cash = Company::Get(_current_company)->money;
		const auto random = std::array{_random.state[0], _random.state[1]};
		const auto quote = PreviewTBMExtension(_current_company, id, start, end);
		INFO("native extension column=" << column << " error=" << quote.result.GetErrorMessage().base() << " plan=" << to_underlying(quote.plan.error));
		CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(Company::Get(_current_company)->money == cash);
		CHECK(std::array{_random.state[0], _random.state[1]} == random); REQUIRE(quote.result.Succeeded());
		const auto original_job = project->job; const auto original_cutter = project->cutter;
		REQUIRE(Command<Commands::ExtendTBMProject>::Do(DoCommandFlag::Execute, id, start, end, quote.job_revision, quote.fingerprint, ExcavationPlanIntent::Area).Succeeded());
		CHECK(GetTBMProject(id)->job == original_job); CHECK(GetTBMProject(id)->cutter == original_cutter);
		REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, id, TBMProjectAction::Resume).Succeeded());
		fmt::print("FIRST_PLANT_EXTENSION column={} cursor={} route={}\n", column, GetExcavationJob(original_job)->cursor, GetExcavationJob(original_job)->route.size());
		return true;
	}
	return false;
}

/** @param arguments Exact native startup console invocation. @return Success after inspecting the actual reconstructed original world. */
static bool ObserveFirstPlantFile(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_first_plant_file_observer != nullptr); (*_first_plant_file_observer)(); return true;
}

/** Inspect the original paid floor/work/hauler world through actual application afterload and independent copied preview, retaining an ordinary native resave. */
TEST_CASE("Paid first-plant originals survive ordinary native afterload and copied preview", "[.geothermal-first-plant-native-file]")
{
	const char *input = std::getenv("OTTD_FIRST_PLANT_INPUT"); REQUIRE(input != nullptr);
	const char *action_text = std::getenv("OTTD_FIRST_PLANT_ACTION");
	const std::string_view action = action_text == nullptr ? "inspect" : action_text;
	REQUIRE((action == "inspect" || action == "continue" || action == "release")); // Each native action either preserves owners, advances the real loop, or releases already settled work through ordinary paid commands.
	const auto paths = GetTestCaseArtifactPaths("geothermal", "first-plant");
	bool observed = false, started = false, completed = false; std::exception_ptr failure;
	TBMProjectID running{UINT32_MAX}; uint32_t frames = 0, initial_cursor = 0, initial_exchanges = 0;
	ExcavationCutResult expected;
	Money initial_cash = 0; std::array<Expenses, 3> initial_expenses{}; int initial_year = 0;
	/** Inspect and resave the actual loaded authorities; no fixture constructor, terrain restoration or replacement participant is involved. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
			REQUIRE(GetGeologyWorldState().vents.size() == 1);
			CHECK(GetGeologyWorldState().vents.front().thermal_capacity_w == 1000000);
			CHECK(CaptureGeothermalGenerators().empty()); CHECK(CaptureGeothermalCollectors().empty());
			CHECK(CaptureGeothermalRisers().empty()); CHECK(CaptureUtilityPowerState().producers.empty());
			const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
			const auto projects = ComputeTBMProjectDigest(), exchange = ComputeTBMExchangeDigest();
			const auto cash = Company::Get(_current_company)->money; const auto clock = TimerGameTick::counter;
			const auto random = std::array{_random.state[0], _random.state[1]};
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			CHECK(ComputeTBMProjectDigest() == projects); CHECK(ComputeTBMExchangeDigest() == exchange);
			CHECK(Company::Get(_current_company)->money == cash); CHECK(TimerGameTick::counter == clock);
			CHECK(std::array{_random.state[0], _random.state[1]} == random);
			const auto saved = paths.Output("saves", "first-plant-inspect.sav");
			SaveLoadedFirstPlant(saved);
			fmt::print("FIRST_PLANT_READ action=inspect cash={} path={}\n", cash.base(), saved.string());
			if (action == "release") {
				const auto active = CaptureTBMProjects(); REQUIRE(active.size() == 1);
				const auto quote = QuoteLoadedFirstPlant(active.front());
				CheckLoadedFirstPlantComplete(active.front(), quote); CheckLoadedFirstPlantSpoil(_current_company);
				ReleaseLoadedFirstPlant(active.front().id);
				CheckLoadedFirstPlantSpoil(_current_company);
				const auto ready = paths.Output("saves", "first-plant-ready-for-delivery.sav"); SaveLoadedFirstPlant(ready);
				fmt::print("FIRST_PLANT_READ action=release cash={} path={}\n", Company::Get(_current_company)->money.base(), ready.string());
				completed = true;
			}
			if (action == "continue") {
				const auto active = CaptureTBMProjects(); REQUIRE(active.size() == 1); running = active.front().id;
				initial_cursor = GetExcavationJob(active.front().job)->cursor;
				const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
				initial_exchanges = assignments.front().exchanges;
				TraceLoadedFirstPlantRoute(active.front(), assignments.front());
				expected = QuoteLoadedFirstPlant(active.front()); CheckLoadedFirstPlantSpoil(_current_company);
				initial_cash = Company::Get(_current_company)->money; initial_expenses = Company::Get(_current_company)->yearly_expenses;
				initial_year = TimerGameEconomy::year.base();
				if (active.front().blocked == TBMBlockedReason::Paused) REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, running, TBMProjectAction::Resume).Succeeded());
				REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded());
				started = true; observed = true;
				return; // Only the ordinary native loop advances the loaded world after this startup command.
			}
		} catch (...) { failure = std::current_exception(); }
		observed = true; _exit_game = true;
	};
	/** @param elapsed Actual native game frames since the previous callback; this observer never advances a simulation owner itself. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&](uint elapsed) {
		if (!started) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; frames += elapsed;
			if (ExtendLoadedFirstPlant(running)) expected = QuoteLoadedFirstPlant(*GetTBMProject(running));
			const auto *project = GetTBMProject(running); REQUIRE(project != nullptr);
			const auto assignments = CaptureTBMExchangeAssignments(); REQUIRE(assignments.size() == 1);
			REQUIRE_FALSE(Train::Get(project->cutter)->vehstatus.Test(VehState::Crashed));
			REQUIRE_FALSE(Train::Get(assignments.front().hauler)->vehstatus.Test(VehState::Crashed));
			if (frames < 8000) return;
			const auto &job = *GetExcavationJob(project->job);
			// A continued burst must earn real paid work or another actual wagon
			// exchange; advancing only a clock cannot count as useful continuation.
			if (job.cursor == initial_cursor && assignments.front().exchanges == initial_exchanges) TraceLoadedFirstPlantRoute(*project, assignments.front()); // Preserve the actual destination, whole physical chains and native admitted graph at the stall rather than infer a cause from the phase alone.
			CHECK((job.cursor > initial_cursor || assignments.front().exchanges > initial_exchanges));
			CheckLoadedFirstPlantSpoil(_current_company);
			const auto *company = Company::Get(_current_company); Money spent = 0;
			// Each8,000-frame burst spans less than one economy year. Include an
			// oldest bucket dropped by the actual annual rotation, preserving the
			// complete paid history even when the full plant takes several years.
			const int years = TimerGameEconomy::year.base() - initial_year; REQUIRE((years == 0 || years == 1));
			for (size_t year = 0; year < initial_expenses.size(); ++year) {
				for (size_t category = 0; category < initial_expenses[year].size(); ++category) {
					const auto kind = static_cast<ExpensesType>(category);
					spent += company->yearly_expenses[year][kind] - initial_expenses[year][kind];
					if (years == 1 && year == 2) spent += initial_expenses[year][kind]; // An actual ledger rotation must not erase previously billed cash from the comparison.
				}
				CHECK(company->yearly_expenses[year][ExpensesType::TrainRevenue] == Money{0});
			}
			CHECK(company->money == initial_cash - spent);
			fmt::print("FIRST_PLANT_CASH before={} spent={} after={} year_rotations={}\n", initial_cash.base(), spent.base(), company->money.base(), years);
			if (job.cursor == job.route.size()) {
				CheckLoadedFirstPlantComplete(*project, expected);
			}
			if (!IsTBMProjectFinished(project->phase)) REQUIRE(Command<Commands::ControlTBMProject>::Do(DoCommandFlag::Execute, running, TBMProjectAction::Pause).Succeeded());
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
			const auto saved = paths.Output("saves", "first-plant-continued.sav"); SaveLoadedFirstPlant(saved);
			fmt::print("FIRST_PLANT_READ action=continue cash={} path={}\n", Company::Get(_current_company)->money.base(), saved.string());
			fmt::print("FIRST_PLANT_CONTINUED frames={} cursor={}/{} earth={} rock={} exchanges={} phase={} wait={}\n", frames,
				job.cursor, job.route.size(), job.completed_earth_t, job.completed_rock_t, assignments.front().exchanges,
				to_underlying(project->phase), to_underlying(assignments.front().wait));
			completed = true; started = false; _exit_game = true;
		} catch (...) { failure = std::current_exception(); started = false; _exit_game = true; }
	});
	AutoRestoreBackup observer{_first_plant_file_observer, &inspect};
	IConsole::CmdRegister("first_plant_check", ObserveFirstPlantFile);
	const std::string config = GetTestRuntimeConfig().string();
	const std::string_view video = action == "continue" ? "null:ticks=8005" : "null:ticks=2";
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", video, "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
	if (action != "inspect") CHECK(completed);
}
