/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_construction.cpp Actual native construction on the retained paid first-plant world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8FBA-8FBC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-e-first-plant-bootstrap.md.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_first_plant_fixture.h"
#include "test_artifact_paths.h"
#include "../console_internal.h"
#include "../openttd.h"
#include "../landscape_cmd.h"
#include "../train_cmd.h"
#include "../vehicle_cmd.h"
#include "../order_cmd.h"
#include "../misc_cmd.h"
#include "../timer/timer.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_first_plant_build_observer = nullptr; ///< Invocation-local callback; no replacement saved owner or quantity.

/** Actual command-created identities retained only for observation during this native invocation. */
struct FirstPlantConstructionParticipants {
	IndustryID collector = IndustryID::Invalid(); ///< Original funded natural-vent collector.
	IndustryID powerhouse = IndustryID::Invalid(); ///< Original funded1980 generator.
	std::array<IndustryID, 2> risers{}; ///< Separately funded Underground and Deep original-portal machinery.
	StationID origin = StationID::Invalid(); ///< Retained actual owned Surface packet source.
	StationID upper_stop = StationID::Invalid(); ///< Actual same-layer upper-riser catchment.
	StationID deep_stop = StationID::Invalid(); ///< Actual same-layer three-machine Deep catchment.
	VehicleID old_hauler = VehicleID::Invalid(); ///< Retained empty spoil train, withdrawn by its ordinary depot order.
	VehicleID carrier = VehicleID::Invalid(); ///< Newly paid model4 locomotive with four actual model5 holds.
};

/**
 * Execute one complete native command and observe its single already-settled invoice.
 * @tparam Tcmd Existing synchronized command with normal ownership, floor and money admission.
 * @tparam Targs Its public typed arguments.
 * @param invoices Observed command costs, never a replacement company balance.
 * @param args Exact actual native inputs.
 * @return The command's complete native result, including actual created identities.
 */
template <Commands Tcmd, typename... Targs>
static auto FundFirstPlantConstruction(Money &invoices, Targs... args)
{
	auto result = Command<Tcmd>::Do(DoCommandFlag::Execute, args...);
	UpdateSignalsInBuffer();
	const auto &cost = ExtractCommandCost(result);
	INFO("first-plant build command=" << GetCommandName(Tcmd) << " error=" << cost.GetErrorMessage().base());
	REQUIRE(cost.Succeeded()); invoices += cost.GetCost();
	fmt::print("FIRST_PLANT_BUILD_INVOICE command={} cost={}\n", GetCommandName(Tcmd), cost.GetCost().base());
	return result;
}

/**
 * Remove only owned released rail where an actual funded building needs vacant already-earned floor.
 * @param invoices Observation of ordinary clearing invoices, including native refunds.
 * @param anchor Actual same-layer top-left floor cell.
 * @param width Full actual native footprint width.
 * @param height Full actual native footprint height.
 */
static void ClearFirstPlantConstructionFloor(Money &invoices, TileIndex anchor, uint width, uint height)
{
	// Full footprint inspection prevents a foundation from silently opening a
	// corner, destroying another building or replacing protected work support.
	for (uint y = 0; y < height; ++y) {
		for (uint x = 0; x < width; ++x) {
			const auto tile = TileAddWrap(anchor, x, y);
			REQUIRE(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
			REQUIRE(GetTBMSupportTile(tile) == nullptr);
			if (IsPlainRailTile(tile)) {
				REQUIRE(GetTileOwner(tile) == _current_company);
				FundFirstPlantConstruction<Commands::LandscapeClear>(invoices, tile);
			}
			REQUIRE(IsTileType(tile, TileType::Clear));
		}
	}
}

/**
 * Fund native pieces joining actual neighboring route cells on earned floor.
 * @param invoices Observed native track invoices.
 * @param path Explicit same-layer route including existing endpoint housings/approaches.
 */
static void FundFirstPlantFreightPath(Money &invoices, const std::vector<TileIndex> &path)
{
	// Each actual edge pair selects one ordinary native track piece. Existing
	// compatible rails and paid station platforms are reused without rebilling.
	for (size_t index = 1; index + 1 < path.size(); ++index) {
		const auto tile = path[index];
		REQUIRE(GetTileLayer(tile) == GetTileLayer(path.front()));
		REQUIRE(GetGeologyWorldState().terrain.Get(tile) == ExcavationState::Excavated);
		const auto entry = DiagdirBetweenTiles(tile, path[index - 1]);
		const auto exit = DiagdirBetweenTiles(tile, path[index + 1]);
		Track piece = Track::Invalid;
		// Native directed pieces define connectivity; a drawn corner or adjacent
		// coordinate alone cannot provide a route to either physical transition.
		for (Trackdir direction : DiagdirReachesTrackdirs(ReverseDiagDir(entry))) {
			if (TrackdirToExitdir(direction) == exit) piece = TrackdirToTrack(direction);
		}
		REQUIRE(piece != Track::Invalid);
		if (IsRailStationTile(tile)) {
			CHECK(AxisToTrack(GetRailStationAxis(tile)) == piece);
			continue;
		}
		// Only an exact already-paid piece avoids another real construction
		// invoice; a missing branch must pass ordinary native admission.
		if (IsPlainRailTile(tile) && GetTrackBits(tile).Test(piece)) continue;
		FundFirstPlantConstruction<Commands::BuildRail>(invoices, tile, RAILTYPE_RAIL, piece, false);
	}
}

/**
 * Fund four empty bills, real stations/track/fleet and ordinary withdrawal of the retained spoil hauler.
 * @param layout Original source/footprint description from the actual loaded geology.
 * @param invoices Observed real command invoices.
 * @return Actual native participants; no floor, stock, energy or train position is supplied by a fixture.
 */
static FirstPlantConstructionParticipants PrepareFirstPlantConstruction(const FirstPlantLayout &layout, Money &invoices)
{
	FirstPlantConstructionParticipants result;
	// The ready original has exactly one non-cutter engine. Resolve that
	// retained train before buying the separate conventional construction fleet.
	for (Train *train : Train::Iterate()) {
		if (!train->IsFrontEngine() || IsTBMCutterEngine(train->engine_type)) continue; // Wagon units and the two retained paid cutters cannot substitute for the old ordinary hauler.
		REQUIRE(result.old_hauler == VehicleID::Invalid()); result.old_hauler = train->index;
	}
	REQUIRE(result.old_hauler != VehicleID::Invalid());
	auto *old = Train::Get(result.old_hauler);
	REQUIRE(old->GetNumOrders() == 1); result.origin = old->GetOrder(0)->GetDestination().ToStationID();
	REQUIRE(GetTileLayer(Station::Get(result.origin)->xy) == MapLayer::Surface);
	for (const auto &[anchor, side] : std::array{std::pair{layout.collector, 3U}, std::pair{layout.powerhouse, 4U},
		std::pair{layout.upper_riser, 2U}, std::pair{layout.deep_riser, 2U}}) ClearFirstPlantConstructionFloor(invoices, anchor, side, side);
	// Cold public claims retain the actual vent/site. Each native funding
	// command consumes only its blueprint and charges its own complete bill.
	for (const auto &[anchor, kind] : std::array{std::pair{layout.collector, MultiLayerGeothermalConstructionKind::Collector},
		std::pair{layout.powerhouse, MultiLayerGeothermalConstructionKind::Powerhouse}}) {
		const GeothermalSiteRequest request{layout.vent.id, _current_company, kind, anchor, anchor, DiagDirection::SE};
		const auto claim = ClaimGeothermalSite(request);
		INFO("first-plant native site error=" << to_underlying(claim.error)); REQUIRE(claim.error == GeothermalError::None);
		if (kind == MultiLayerGeothermalConstructionKind::Collector) {
			FundFirstPlantConstruction<Commands::BuildGeothermalCollector>(invoices, anchor, claim.id, uint32_t{123});
			result.collector = FindGeothermalCollectorForVent(layout.vent.id);
		} else {
			FundFirstPlantConstruction<Commands::BuildGeothermalGenerator>(invoices, anchor, claim.id, uint32_t{123});
			result.powerhouse = Industry::GetByTile(anchor)->index;
		}
	}
	const std::array sending{GetLayerPortalPairTile(layout.upper_export), GetLayerPortalPairTile(layout.deep_export)};
	const std::array anchors{layout.upper_riser, layout.deep_riser}, ports{layout.upper_riser_port, layout.deep_riser_port};
	// Separate original sending housings each require their own paid2x2 bill;
	// one completed relay can never stand in for the other physical transition.
	for (size_t index = 0; index < sending.size(); ++index) {
		FundFirstPlantConstruction<Commands::BuildGeothermalRiser>(invoices, anchors[index], sending[index], ports[index], DiagDirection::SE, uint32_t{123});
		result.risers[index] = FindGeothermalRiserForPortal(sending[index]);
	}
	const uint x = layout.vent.x, y = layout.vent.y;
	const auto upper_stop = TileXYL(x + 5, y - 6, MapLayer::Underground);
	const auto deep_stop = TileXYL(x + 3, y + 2, MapLayer::DeepUnderground);
	ClearFirstPlantConstructionFloor(invoices, upper_stop, 1, 3);
	ClearFirstPlantConstructionFloor(invoices, deep_stop, 1, 5);
	for (const auto &[anchor, length] : std::array{std::pair{upper_stop, uint8_t{3}}, std::pair{deep_stop, uint8_t{5}}}) {
		FundFirstPlantConstruction<Commands::BuildRailStation>(invoices, anchor, RAILTYPE_RAIL, Axis::Y, uint8_t{1}, length, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false);
	}
	result.upper_stop = Station::GetByTile(upper_stop)->index; result.deep_stop = Station::GetByTile(deep_stop)->index;
	std::vector<TileIndex> upper_path{TileXYL(x + 1, y - 10, MapLayer::Underground), TileXYL(x + 1, y - 9, MapLayer::Underground)};
	// Follow the earned chamber's north/east rim, outside both upper machines,
	// then the real station and original D door; no extra rock is opened.
	for (uint column = x + 1; column <= x + 5; ++column) upper_path.push_back(TileXYL(column, y - 8, MapLayer::Underground));
	for (uint row = y - 7; row <= y - 3; ++row) upper_path.push_back(TileXYL(x + 5, row, MapLayer::Underground));
	for (uint row = y - 3; row <= y - 1; ++row) upper_path.push_back(TileXYL(x + 4, row, MapLayer::Underground));
	FundFirstPlantFreightPath(invoices, upper_path);
	std::vector<TileIndex> deep_path{sending[1], TileXYL(x + 4, y, MapLayer::DeepUnderground), TileXYL(x + 3, y, MapLayer::DeepUnderground)};
	// One real Deep catchment reaches all three distinct native facilities;
	// its platform is outside their machinery and leaves their ports exposed.
	for (uint row = y + 1; row <= y + 7; ++row) deep_path.push_back(TileXYL(x + 3, row, MapLayer::DeepUnderground));
	FundFirstPlantFreightPath(invoices, deep_path);
	const auto depot = TileXYL(x + 1, y - 20, MapLayer::Surface);
	REQUIRE(IsRailDepotTile(depot));
	const auto [cost, engine, capacity, mail, capacities] = FundFirstPlantConstruction<Commands::BuildVehicle>(invoices, depot, GetUndergroundEngine(4), false, INVALID_CARGO, ClientID::Invalid);
	result.carrier = engine;
	// Four separate native holds retain each canonical label's actual capacity;
	// neither the locomotive nor process-local bookkeeping provides extra stock.
	for (CargoLabel label : {CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}}) {
		const auto cargo = GetCargoTypeByLabel(label);
		REQUIRE(Station::Get(result.upper_stop)->goods[cargo].status.Test(GoodsEntry::State::Acceptance));
		REQUIRE(Station::Get(result.deep_stop)->goods[cargo].status.Test(GoodsEntry::State::Acceptance));
		const auto [wagon_cost, wagon, wagon_capacity, wagon_mail, wagon_capacities] = FundFirstPlantConstruction<Commands::BuildVehicle>(invoices, depot, GetUndergroundEngine(5), false, cargo, ClientID::Invalid);
		CHECK(wagon_capacity == 64);
		FundFirstPlantConstruction<Commands::MoveRailVehicle>(invoices, wagon, Train::Get(engine)->Last()->index, false);
	}
	// The native order list remains the only travel authority for purchases and
	// both deliveries; no destination or arrival is written into vehicle fields.
	for (const auto station : {result.origin, result.upper_stop, result.deep_stop}) {
		Order order; order.MakeGoToStation(station);
		order.SetUnloadType(station == result.origin ? OrderUnloadType::NoUnload : OrderUnloadType::UnloadIfPossible);
		order.SetLoadType(station == result.origin ? OrderLoadType::LoadIfPossible : OrderLoadType::NoLoad);
		FundFirstPlantConstruction<Commands::InsertOrder>(invoices, engine, Train::Get(engine)->GetNumOrders(), order);
	}
	FundFirstPlantConstruction<Commands::SendVehicleToDepot>(invoices, old->index, DepotCommandFlags{}, VehicleListIdentifier{});
	if (old->vehstatus.Test(VehState::Stopped)) FundFirstPlantConstruction<Commands::StartStopVehicle>(invoices, old->index, true);
	return result;
}

/** @param path Fresh native output owned by this invocation. Persist actual world state without altering stock, work, vehicles or money. */
static void SaveFirstPlantConstruction(const std::filesystem::path &path)
{
	AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Inspect actual funded bill owners; preparation cannot create stock, labour or an electrical source. */
static void CheckFirstPlantPreparedBills()
{
	const auto collectors = CaptureGeothermalCollectors();
	const auto generators = CaptureGeothermalGenerators();
	const auto risers = CaptureGeothermalRisers();
	REQUIRE(collectors.size() == 1); REQUIRE(generators.size() == 1); REQUIRE(risers.size() == 2);
	std::vector<std::pair<IndustryID, MultiLayerGeothermalConstructionKind>> bills;
	// Each retained native incarnation supplies its own stock/work owner. The
	// inspection never substitutes a total bill counter or advances construction.
	for (const auto &state : collectors) {
		CHECK(state.work_ticks == 0); CHECK_FALSE(state.materials_committed);
		bills.emplace_back(state.industry, MultiLayerGeothermalConstructionKind::Collector);
	}
	for (const auto &state : generators) {
		CHECK(state.work_ticks == 0); CHECK_FALSE(state.materials_committed);
		bills.emplace_back(state.industry, MultiLayerGeothermalConstructionKind::Powerhouse);
	}
	for (const auto &state : risers) {
		CHECK(state.work_ticks == 0); CHECK_FALSE(state.materials_committed);
		bills.emplace_back(state.industry, MultiLayerGeothermalConstructionKind::Riser);
	}
	// Funding and unrelated normal frames cannot supply hidden accepted stock.
	for (const auto &[industry, kind] : bills) {
		for (const auto &entry : GetMultiLayerGeothermalConstruction(kind)->inputs) {
			CHECK(Industry::Get(industry)->GetCargoAccepted(GetCargoTypeByLabel(entry.label))->waiting == 0);
		}
	}
	CHECK(CaptureUtilityPowerState().producers.empty());
}

/** @param arguments Actual native startup-console invocation. @return Success after observing the reconstructed original world. */
static bool ObserveFirstPlantConstruction(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_first_plant_build_observer != nullptr); (*_first_plant_build_observer)(); return true;
}

/** Real afterload funds empty native bills and freight infrastructure, then ordinary movement withdraws the original spoil train. */
TEST_CASE("Paid first-plant construction uses the original earned native world", "[.geothermal-first-plant-construction-file]")
{
	const char *input = std::getenv("OTTD_FIRST_PLANT_INPUT"); REQUIRE(input != nullptr);
	const char *action_text = std::getenv("OTTD_FIRST_PLANT_ACTION"); REQUIRE(action_text != nullptr);
	const std::string_view action{action_text};
	REQUIRE((action == "prepare" || action == "inspect")); // Preparation spends through native commands; inspection only verifies and resaves the existing four empty bills.
	const auto paths = GetTestCaseArtifactPaths("geothermal", "first-plant");
	bool started = false, completed = false; uint frames = 0; std::exception_ptr failure;
	FirstPlantConstructionParticipants participants;
	Money initial_cash = 0, invoices = 0; uint64_t geology = 0;
	std::array<Expenses, 3> initial_expenses{}; int initial_year = 0;
	/** Actual afterload callback; no fixture world is constructed and every change uses the original native owners. */
	std::function<void()> prepare = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company};
			REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error)); REQUIRE(GetGeologyWorldState().vents.size() == 1);
			REQUIRE(CaptureTBMProjects().empty()); REQUIRE(CaptureTBMExchangeAssignments().empty()); REQUIRE(CaptureTBMSupport().empty());
			if (action == "prepare") {
				REQUIRE(CaptureGeothermalCollectors().empty()); REQUIRE(CaptureGeothermalGenerators().empty()); REQUIRE(CaptureGeothermalRisers().empty());
			} else {
				CheckFirstPlantPreparedBills();
			}
			REQUIRE(GetSurfaceSupplyOrderHistory().empty()); REQUIRE(CargoPacket::GetNumItems() == 0);
			geology = ComputeGeologyStateDigest(GetGeologyWorldState()); initial_cash = Company::Get(_current_company)->money;
			initial_expenses = Company::Get(_current_company)->yearly_expenses; initial_year = TimerGameEconomy::year.base();
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(Company::Get(_current_company)->money == initial_cash);
			const auto inspected = paths.Output("saves", "first-plant-inspect.sav"); SaveFirstPlantConstruction(inspected);
			fmt::print("FIRST_PLANT_READ action=inspect cash={} path={}\n", initial_cash.base(), inspected.string());
			if (action == "inspect") { completed = true; _exit_game = true; return; }
			participants = PrepareFirstPlantConstruction(DescribeFirstPlantLayout(GetGeologyWorldState().vents.front()), invoices);
			CHECK(Company::Get(_current_company)->money == initial_cash - invoices);
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded()); started = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	/** @param elapsed Actual normal game frames; this observer never advances native movement, loading or construction itself. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&](uint elapsed) {
		if (!started) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; frames += elapsed;
			REQUIRE_FALSE(Train::Get(participants.old_hauler)->vehstatus.Test(VehState::Crashed));
			REQUIRE(Train::Get(participants.carrier)->IsStoppedInDepot());
			// Normal depot arrival clears the old Surface platform before freight
			// starts. A bounded timeout diagnoses a real obstruction, never an arrival.
			if (!Train::Get(participants.old_hauler)->IsStoppedInDepot()) {
				INFO("old hauler " << DescribeTrain(Train::Get(participants.old_hauler))); REQUIRE(frames < 4000); return;
			}
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			CHECK(GetSurfaceSupplyOrderHistory().empty()); CHECK(CargoPacket::GetNumItems() == 0);
			CheckFirstPlantPreparedBills();
			const auto *company = Company::Get(_current_company); Money spent = 0;
			const int years = TimerGameEconomy::year.base() - initial_year; REQUIRE((years == 0 || years == 1)); // The bounded withdrawal can cross one actual year boundary, whose oldest expense bucket must remain in the cash comparison.
			// Observe every actual expense category, preserving an oldest bucket
			// dropped by native annual rotation; command totals provide no free cash.
			for (size_t year = 0; year < initial_expenses.size(); ++year) {
				for (size_t category = 0; category < initial_expenses[year].size(); ++category) {
					const auto kind = static_cast<ExpensesType>(category);
					spent += company->yearly_expenses[year][kind] - initial_expenses[year][kind];
					if (years == 1 && year == 2) spent += initial_expenses[year][kind]; // Native year rollover cannot erase already paid history.
				}
				CHECK(company->yearly_expenses[year][ExpensesType::TrainRevenue] == Money{0});
			}
			CHECK(company->money == initial_cash - spent); CHECK(spent >= invoices);
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
			const auto saved = paths.Output("saves", "first-plant-construction-prepared.sav"); SaveFirstPlantConstruction(saved);
			fmt::print("FIRST_PLANT_BUILD action=prepare cash={} path={}\n", company->money.base(), saved.string());
			fmt::print("FIRST_PLANT_BUILD_READY frames={} invoices={} spent={} collector={} powerhouse={} upper_riser={} deep_riser={} carrier={} old_hauler={} origin={} upper_stop={} deep_stop={}\n",
				frames, invoices.base(), spent.base(), participants.collector.base(), participants.powerhouse.base(), participants.risers[0].base(), participants.risers[1].base(),
				participants.carrier.base(), participants.old_hauler.base(), participants.origin.base(), participants.upper_stop.base(), participants.deep_stop.base());
			completed = true; started = false; _exit_game = true;
		} catch (...) { failure = std::current_exception(); started = false; _exit_game = true; }
	});
	AutoRestoreBackup observer{_first_plant_build_observer, &prepare}; IConsole::CmdRegister("first_plant_build", ObserveFirstPlantConstruction);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "null:ticks=4005", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(completed);
}
