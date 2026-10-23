/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_utilities.cpp Actual paid physical networks on the original delivered first-plant world.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FBC.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-e-first-plant-bootstrap.md.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_first_plant_fixture.h"
#include "test_artifact_paths.h"
#include "../geothermal_utility.h"
#include "../geothermal_utility_cmd.h"
#include "../console_internal.h"
#include "../openttd.h"
#include "../vehicle_cmd.h"
#include "../misc_cmd.h"
#include "../timer/timer.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static std::function<void()> *_first_plant_utility_observer = nullptr; ///< Invocation-local startup observer; no saved controller or utility authority.

/** Desired route geometry only; native paid links exclusively own installed identity, capacity and quantity. */
struct FirstPlantUtilityPaths {
	std::vector<TileIndex> heat; ///< Original Deep collector port to original generator port, around actual machinery/platforms.
	std::vector<TileIndex> deep; ///< Actual Deep generator-to-sending-riser cable cells.
	std::vector<TileIndex> upper; ///< Receiving Underground housing to the separate upper sending-riser port.
};

/** @param layout Original source and paid machinery geometry. @return Explicit cardinal paths on already earned floor; this creates no physical link. */
static FirstPlantUtilityPaths DescribeFirstPlantUtilityPaths(const FirstPlantLayout &layout)
{
	FirstPlantUtilityPaths paths; const uint x = layout.vent.x, y = layout.vent.y;
	paths.heat.push_back(layout.collector);
	// Go north around the lower riser, then east around D's Heat-ineligible
	// housing and the actual Deep platform; every requested cell was paid by8FA.
	for (int row = static_cast<int>(y) - 1; row >= static_cast<int>(y) - 3; --row) paths.heat.push_back(TileXYL(x, row, MapLayer::DeepUnderground));
	for (uint column = x + 1; column <= x + 4; ++column) paths.heat.push_back(TileXYL(column, y - 3, MapLayer::DeepUnderground));
	paths.heat.push_back(TileXYL(x + 4, y - 2, MapLayer::DeepUnderground));
	for (uint row = y - 2; row <= y + 1; ++row) paths.heat.push_back(TileXYL(x + 5, row, MapLayer::DeepUnderground));
	paths.heat.push_back(TileXYL(x + 4, y + 1, MapLayer::DeepUnderground)); paths.heat.push_back(layout.powerhouse);
	paths.deep = {layout.powerhouse, TileXYL(x + 4, y + 1, MapLayer::DeepUnderground), TileXYL(x + 4, y, MapLayer::DeepUnderground),
		TileXYL(x + 3, y, MapLayer::DeepUnderground), layout.deep_riser_port};
	// The relay starts at the real receiving D housing and remains off the
	// station's east column. The separate upper riser owns the later Surface hop.
	for (int row = static_cast<int>(y) - 1; row >= static_cast<int>(y) - 7; --row) paths.upper.push_back(TileXYL(x + 4, row, MapLayer::Underground));
	paths.upper.push_back(layout.upper_riser_port);
	return paths;
}

/** Inspect complete bills and retain the original unused collector Heat meter before the first electrical operating interval. */
static void CheckFirstPlantUtilityHardware()
{
	const auto collectors = CaptureGeothermalCollectors(); const auto generators = CaptureGeothermalGenerators(); const auto risers = CaptureGeothermalRisers();
	REQUIRE(collectors.size() == 1); REQUIRE(generators.size() == 1); REQUIRE(risers.size() == 2);
	CHECK(collectors.front().materials_committed); CHECK(collectors.front().work_ticks == 296);
	CHECK(generators.front().model == GeothermalGeneratorModel::VentTap); CHECK(generators.front().materials_committed); CHECK(generators.front().work_ticks == 592);
	CHECK(generators.front().active_service_ticks == 0);
	for (const auto &riser : risers) { CHECK(riser.materials_committed); CHECK(riser.work_ticks == 296); } // Each original transition has its own consumed bill and complete native work; one cannot stand in for the other.
	const auto power = CaptureUtilityPowerState(); REQUIRE(power.producers.size() == 1);
	const auto &source = power.producers.front();
	// The completed collector already publishes its original bounded thermal
	// offer through the common driver. Preserve that one real Heat meter and
	// its history; no cable, electrical source, reservation or use exists yet.
	CHECK(source.kind == UtilityGenerationKind::Geothermal); CHECK(source.meter.domain == UtilityEnergyDomain::Heat);
	CHECK(source.meter.layer == MapLayer::DeepUnderground); CHECK(source.owner == _current_company);
	CHECK(source.reserved_j == 0); CHECK(source.consumed_lifetime_j == 0); CHECK(CargoPacket::GetNumItems() == 0);
}

/** @param path Fresh invocation-owned output. Persist actual native owners without changing work, energy or their pause policy. */
static void SaveFirstPlantUtilities(const std::filesystem::path &path)
{
	AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}}; AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/**
 * Reconcile actual cash against every original native expense category.
 * @param before Cash at ordinary afterload before commands or normal frames.
 * @param expenses All three original retained annual expense buckets.
 * @param initial_year Original economy year used only to account for a real bucket rotation.
 * @return Actual paid spending; this neither bills nor changes company money.
 */
static Money CheckFirstPlantUtilityCash(Money before, const std::array<Expenses, 3> &expenses, int initial_year)
{
	const auto *company = Company::Get(_current_company); Money spent = 0;
	const int years = TimerGameEconomy::year.base() - initial_year; REQUIRE((years == 0 || years == 1)); // A bounded withdrawal can cross one year; the dropped oldest bucket must not erase previously paid costs.
	// Every native expense category participates, including zero freight income
	// and a bucket dropped by the actual annual rotation during ordinary travel.
	for (size_t year = 0; year < expenses.size(); ++year) {
		for (size_t category = 0; category < expenses[year].size(); ++category) {
			const auto kind = static_cast<ExpensesType>(category); spent += company->yearly_expenses[year][kind] - expenses[year][kind];
			if (years == 1 && year == 2) spent += expenses[year][kind]; // Preserve actual history across native annual rotation.
		}
		CHECK(company->yearly_expenses[year][ExpensesType::TrainRevenue] == Money{0});
	}
	CHECK(company->money == before - spent); return spent;
}

/**
 * Install one exact missing native domain cell; an already retained owned cell is reused without repurchase.
 * @param tile Actual paid floor or original declared machinery/housing port.
 * @param domain Separate physical Heat or electrical network.
 * @param invoices Observation of native command charges, never a money owner.
 * @return Whether this invocation actually bought a new cell.
 */
static bool FundFirstPlantUtilityCell(TileIndex tile, UtilityEnergyDomain domain, Money &invoices)
{
	const auto *terms = GetMultiLayerLocalUtilityLinkContract(domain); REQUIRE(terms != nullptr);
	if (const auto existing = GetGeothermalUtilityLink(tile, domain)) {
		CHECK(existing->owner == _current_company); CHECK(existing->capacity_w == terms->capacity_w); return false;
	}
	const auto money = Company::Get(_current_company)->money; const auto utility = ComputeUtilityPowerDigest();
	const auto quote = PrepareGeothermalUtilityLink(tile, domain);
	INFO("native utility cell layer=" << to_underlying(GetTileLayer(tile)) << " x=" << TileX(tile) << " y=" << TilePlaneY(tile) << " domain=" << to_underlying(domain) << " error=" << to_underlying(quote.error));
	CHECK(Company::Get(_current_company)->money == money); CHECK(ComputeUtilityPowerDigest() == utility); REQUIRE(quote.error == GeothermalError::None);
	const auto result = Command<Commands::BuildGeothermalUtilityLink>::Do(DoCommandFlag::Execute, tile, domain); REQUIRE(result.Succeeded());
	CHECK(result.GetCost() == Money{1000}); CHECK(Company::Get(_current_company)->money == money - result.GetCost()); invoices += result.GetCost();
	const auto installed = GetGeothermalUtilityLink(tile, domain); REQUIRE(installed.has_value()); CHECK(installed->id == quote.id);
	fmt::print("FIRST_PLANT_UTILITY_CELL layer={} x={} y={} domain={} id={} cost={}\n", to_underlying(GetTileLayer(tile)), TileX(tile), TilePlaneY(tile), to_underlying(domain), installed->id.base(), result.GetCost().base());
	return true;
}

/** @param paths Original explicit geometry. Verify all paid native endpoint-to-endpoint routes without granting or consuming energy. */
static void CheckFirstPlantUtilityPaths(const FirstPlantUtilityPaths &paths)
{
	// Actual derived topology, not a caller's route vector, establishes each
	// complete physical path and its minimum paid capacity on the proper plane.
	for (const auto &[cells, domain] : std::array{std::pair{&paths.heat, UtilityEnergyDomain::Heat}, std::pair{&paths.deep, UtilityEnergyDomain::Electricity}, std::pair{&paths.upper, UtilityEnergyDomain::Electricity}}) {
		const auto path = FindGeothermalUtilityPath(_current_company, domain, cells->front(), cells->back()); REQUIRE(path.has_value());
		CHECK(path->cells.size() == cells->size()); CHECK(path->capacity_w == (domain == UtilityEnergyDomain::Heat ? 1000000 : 200000));
	}
	CHECK(ValidateGeothermalUtilityLinks());
}

/** @param arguments Actual native startup-console invocation. @return Success after the original loaded world has been observed. */
static bool ObserveFirstPlantUtilities(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_first_plant_utility_observer != nullptr); (*_first_plant_utility_observer)(); return true;
}

/** Original completed material/work survives actual withdrawal, paid physical installation and ordinary file continuation. */
TEST_CASE("First-plant original paid utility paths retain their native hardware and cash", "[.geothermal-first-plant-utility-file]")
{
	const char *input = std::getenv("OTTD_FIRST_PLANT_INPUT"); REQUIRE(input != nullptr);
	const char *action_text = std::getenv("OTTD_FIRST_PLANT_ACTION"); REQUIRE(action_text != nullptr); const std::string_view action{action_text};
	REQUIRE((action == "park" || action == "install" || action == "inspect")); // Travel withdraws the original carrier; installation buys real cells while paused; inspection changes no native owner.
	const auto paths = GetTestCaseArtifactPaths("geothermal", "first-plant");
	bool started = false, completed = false; std::exception_ptr failure; uint frames = 0; VehicleID carrier = VehicleID::Invalid();
	Money initial_cash = 0; std::array<Expenses, 3> initial_expenses{}; int initial_year = 0; uint64_t geology = 0;
	/** Native afterload resolves original participants and never constructs a replacement fixture world. */
	std::function<void()> begin = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error)); CheckFirstPlantUtilityHardware();
			REQUIRE(CaptureTBMProjects().empty()); REQUIRE(CaptureTBMSupport().empty()); REQUIRE(CaptureTBMExchangeAssignments().empty());
			// Both ordinary locomotives share model4; only the original construction
			// carrier has the three manual stops, even after native implicit visits.
			for (Train *train : Train::Iterate()) {
				if (!train->IsFrontEngine() || train->engine_type != GetUndergroundEngine(4) || train->GetNumManualOrders() != 3) continue;
				REQUIRE(carrier == VehicleID::Invalid()); carrier = train->index;
			}
			REQUIRE(carrier != VehicleID::Invalid()); REQUIRE(GetGeologyWorldState().vents.size() == 1);
			const auto geometry = DescribeFirstPlantUtilityPaths(DescribeFirstPlantLayout(GetGeologyWorldState().vents.front()));
			initial_cash = Company::Get(_current_company)->money; initial_expenses = Company::Get(_current_company)->yearly_expenses; initial_year = TimerGameEconomy::year.base();
			geology = ComputeGeologyStateDigest(GetGeologyWorldState()); const auto supply = ComputeSurfaceSupplyDigest(), utility = ComputeUtilityPowerDigest();
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeSurfaceSupplyDigest() == supply); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Company::Get(_current_company)->money == initial_cash);
			const auto inspected = paths.Output("saves", "first-plant-inspect.sav"); SaveFirstPlantUtilities(inspected);
			fmt::print("FIRST_PLANT_READ action=inspect cash={} path={}\n", initial_cash.base(), inspected.string());
			if (action == "inspect") { completed = true; _exit_game = true; return; }
			if (action == "park") {
				REQUIRE(Command<Commands::SendVehicleToDepot>::Do(DoCommandFlag::Execute, carrier, DepotCommandFlags{}, VehicleListIdentifier{}).Succeeded());
				if (Train::Get(carrier)->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, carrier, true).Succeeded());
				REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded()); started = true; return;
			}
			REQUIRE(Train::Get(carrier)->IsStoppedInDepot()); REQUIRE(_pause_mode.Any());
			Money invoices = 0; uint added = 0;
			// Retain a genuine paid prefix before finishing the Heat route. A
			// resumed prefix reuses existing identities instead of billing them twice.
			for (size_t index = 0; index < geometry.heat.size(); ++index) {
				if (FundFirstPlantUtilityCell(geometry.heat[index], UtilityEnergyDomain::Heat, invoices)) ++added;
				if (index == 6) {
					const auto saved = paths.Output("saves", "first-plant-utilities-partial.sav"); SaveFirstPlantUtilities(saved);
					fmt::print("FIRST_PLANT_UTILITY_SAVE stage=partial cash={} path={}\n", Company::Get(_current_company)->money.base(), saved.string());
				}
			}
			for (const auto *network : {&geometry.deep, &geometry.upper}) {
				for (const auto tile : *network) if (FundFirstPlantUtilityCell(tile, UtilityEnergyDomain::Electricity, invoices)) ++added;
			} // Separate Deep and Underground cables each pay their own physical domain cells, including real sending/receiving ports.
			CheckFirstPlantUtilityPaths(geometry); CheckFirstPlantUtilityHardware();
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeSurfaceSupplyDigest() == supply);
			CHECK(Company::Get(_current_company)->money == initial_cash - invoices); CHECK(CheckFirstPlantUtilityCash(initial_cash, initial_expenses, initial_year) == invoices);
			const auto saved = paths.Output("saves", "first-plant-utilities-complete.sav"); SaveFirstPlantUtilities(saved);
			fmt::print("FIRST_PLANT_UTILITY_SAVE stage=complete cash={} path={}\n", Company::Get(_current_company)->money.base(), saved.string());
			fmt::print("FIRST_PLANT_UTILITY_READY action=install added={} invoice={} cash={} heat={} deep={} upper={}\n", added, invoices.base(), Company::Get(_current_company)->money.base(), geometry.heat.size(), geometry.deep.size(), geometry.upper.size());
			completed = true; _exit_game = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	/** @param elapsed Actual normal game frames; ordinary depot movement alone withdraws the retained carrier. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&](uint elapsed) {
		if (!started) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; frames += elapsed; const auto *head = Train::Get(carrier); REQUIRE_FALSE(head->vehstatus.Test(VehState::Crashed));
			// A timeout diagnoses an actual native obstruction; no part is hidden,
			// moved or marked stored by this observer to satisfy withdrawal.
			if (!head->IsStoppedInDepot()) { INFO(DescribeTrain(head)); REQUIRE(frames < 4000); return; }
			CheckFirstPlantUtilityHardware(); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			const auto spent = CheckFirstPlantUtilityCash(initial_cash, initial_expenses, initial_year);
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
			const auto saved = paths.Output("saves", "first-plant-construction-parked.sav"); SaveFirstPlantUtilities(saved);
			fmt::print("FIRST_PLANT_UTILITY_SAVE stage=parked cash={} path={}\n", Company::Get(_current_company)->money.base(), saved.string());
			fmt::print("FIRST_PLANT_UTILITY_READY action=park frames={} spent={} cash={} carrier={}\n", frames, spent.base(), Company::Get(_current_company)->money.base(), carrier.base());
			completed = true; started = false; _exit_game = true;
		} catch (...) { failure = std::current_exception(); started = false; _exit_game = true; }
	});
	AutoRestoreBackup observer{_first_plant_utility_observer, &begin}; IConsole::CmdRegister("first_plant_wire", ObserveFirstPlantUtilities);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "null:ticks=4005", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(completed);
}
