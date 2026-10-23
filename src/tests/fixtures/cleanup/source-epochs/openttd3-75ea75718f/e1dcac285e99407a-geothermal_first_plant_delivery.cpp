/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_first_plant_delivery.cpp Paid Surface packets and normal GameLoop delivery to the original four first-plant bills.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FBB.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-e-first-plant-bootstrap.md.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_first_plant_fixture.h"
#include "test_artifact_paths.h"
#include "../surface_supply_cmd.h"
#include "../console_internal.h"
#include "../openttd.h"
#include "../vehicle_cmd.h"
#include "../misc_cmd.h"
#include "../timer/timer.h"
#include "../saveload/saveload_func.h"
#include "../safeguards.h"

static constexpr std::array<CargoLabel, 4> _first_plant_materials{CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}}; ///< Scenario bill labels resolved through the actual shared cargo registry.
static std::function<void()> *_first_plant_delivery_observer = nullptr; ///< Invocation-local native startup callback, never a saved controller or material owner.

/** Read-only reconciliation of the actual paid history, packet containers and native bill owners. */
struct FirstPlantDeliveryObservation {
	std::array<uint32_t, 4> required{}; ///< Immutable original four-bill requirement, not stock.
	std::array<uint32_t, 4> purchased{}; ///< Actual original SSO quantities, with no replacement refund account.
	std::array<uint32_t, 4> dispatched{}; ///< Actual first-dispatch history.
	std::array<uint32_t, 4> stations{}; ///< Physical units in native station containers.
	std::array<uint32_t, 4> vehicles{}; ///< Physical units in actual vehicles.
	std::array<uint32_t, 4> waiting{}; ///< Actual unconsumed Industry accepted stock.
	std::array<uint32_t, 4> consumed{}; ///< Frozen bills whose actual native consumption receipt is set.
	uint32_t work = 0; ///< Observed native construction work sum, never banked by this observer.
	bool complete = true; ///< Whether all four actual native work owners reached their original bill limit.
};

/** @param label Actual canonical bill meaning. @return Its bounded scenario column; unknown native bill labels fail acceptance. */
static size_t FirstPlantMaterialColumn(CargoLabel label)
{
	const auto it = std::ranges::find(_first_plant_materials, label); REQUIRE(it != _first_plant_materials.end());
	return static_cast<size_t>(it - _first_plant_materials.begin());
}

/**
 * Reconcile all physical native custody against actual purchases and consumed bills.
 * @param origin Original owned Surface pickup retained by the carrier's ordinary order.
 * @return Complete observed quantities; this function advances or publishes nothing.
 */
static FirstPlantDeliveryObservation ObserveFirstPlantMaterials(StationID origin)
{
	FirstPlantDeliveryObservation result;
	/** @param industry Actual bound native bill. @param kind Its original canonical construction meaning. @param committed Actual whole-bill consumption receipt. @param work Actual retained native labour. */
	const auto bill = [&](IndustryID industry, MultiLayerGeothermalConstructionKind kind, bool committed, uint16_t work) {
		const auto *terms = GetMultiLayerGeothermalConstruction(kind); REQUIRE(terms != nullptr);
		result.work += work; result.complete &= work == terms->work_ticks;
		// Consumed quantities derive only from native once-only whole-bill
		// receipts; accepted waiting stock remains exclusively with Industry.
		for (const auto &input : terms->inputs) {
			const auto column = FirstPlantMaterialColumn(input.label);
			result.required[column] += input.quantity;
			result.waiting[column] += Industry::Get(industry)->GetCargoAccepted(GetCargoTypeByLabel(input.label))->waiting;
			if (committed) result.consumed[column] += input.quantity;
		}
	};
	const auto collectors = CaptureGeothermalCollectors(); const auto generators = CaptureGeothermalGenerators(); const auto risers = CaptureGeothermalRisers();
	REQUIRE(collectors.size() == 1); REQUIRE(generators.size() == 1); REQUIRE(risers.size() == 2);
	for (const auto &state : collectors) bill(state.industry, MultiLayerGeothermalConstructionKind::Collector, state.materials_committed, state.work_ticks); // The sole original collector owns its material receipt and work.
	for (const auto &state : generators) {
		REQUIRE(state.model == GeothermalGeneratorModel::VentTap);
		bill(state.industry, MultiLayerGeothermalConstructionKind::Powerhouse, state.materials_committed, state.work_ticks);
		CHECK(state.active_service_ticks == 0); // No paid physical Heat path exists yet, so material delivery cannot secretly operate the powerhouse.
	}
	for (const auto &state : risers) bill(state.industry, MultiLayerGeothermalConstructionKind::Riser, state.materials_committed, state.work_ticks); // Each original physical transition retains its separate bill.
	REQUIRE((result.required == std::array<uint32_t, 4>{28, 20, 10, 6}));
	// Every purchase is still the original owned station's non-refunded supply.
	// Dispatch is history, while actual packets below own remaining units.
	for (const auto &order : GetSurfaceSupplyOrderHistory()) {
		REQUIRE(order.owner == _current_company); REQUIRE(order.station == origin); REQUIRE(order.refunded == 0);
		const auto column = FirstPlantMaterialColumn(order.label);
		result.purchased[column] += order.quantity; result.dispatched[column] += order.dispatched;
	}
	// Inspect every actual container of these four meanings, preventing a
	// nearby station, another vehicle or a reservation from hiding extra stock.
	for (size_t column = 0; column < _first_plant_materials.size(); ++column) {
		const auto cargo = GetCargoTypeByLabel(_first_plant_materials[column]);
		for (const Station *station : Station::Iterate()) result.stations[column] += station->goods[cargo].TotalCount();
		for (const Vehicle *vehicle : Vehicle::Iterate()) {
			if (vehicle->cargo_type == cargo) result.vehicles[column] += vehicle->cargo.TotalCount();
		}
		CHECK(result.purchased[column] == result.stations[column] + result.vehicles[column] + result.waiting[column] + result.consumed[column]);
	}
	return result;
}

/** @param observation Actual reconciled owners. Emit each original cargo meaning without supplying quantities to gameplay. */
static void PrintFirstPlantMaterials(const FirstPlantDeliveryObservation &observation)
{
	// Independent per-label rows expose a missing input that a combined total
	// could conceal, including already consumed whole native construction bills.
	for (size_t column = 0; column < _first_plant_materials.size(); ++column) {
		const auto &label = _first_plant_materials[column];
		fmt::print("FIRST_PLANT_DELIVERY_MATERIAL label={} purchased={} dispatched={} stations={} vehicles={} waiting={} consumed={}\n",
			std::string(label.begin(), label.end()), observation.purchased[column], observation.dispatched[column], observation.stations[column],
			observation.vehicles[column], observation.waiting[column], observation.consumed[column]);
	}
}

/** @param path Fresh invocation-owned output. Save the original world synchronously, retaining real work, packet and vehicle owners. */
static void SaveFirstPlantDelivery(const std::filesystem::path &path)
{
	AutoRestoreBackup pause{_pause_mode, PauseModes{PauseMode::Normal}};
	AutoRestoreBackup compression{_savegame_format, std::string{"lzma"}};
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** @param arguments Actual native startup-console invocation. @return Success after inspecting the reconstructed original world. */
static bool ObserveFirstPlantDelivery(std::span<std::string_view> arguments)
{
	REQUIRE(arguments.size() == 1); REQUIRE(_first_plant_delivery_observer != nullptr); (*_first_plant_delivery_observer)(); return true;
}

/** Actual paid packets travel by the retained ordinary train to native catchments; only normal game frames earn construction work. */
TEST_CASE("First-plant paid material custody and construction use normal native game frames", "[.geothermal-first-plant-delivery-file]")
{
	const char *input = std::getenv("OTTD_FIRST_PLANT_INPUT"); REQUIRE(input != nullptr);
	const char *action_text = std::getenv("OTTD_FIRST_PLANT_ACTION"); REQUIRE(action_text != nullptr); const std::string_view action{action_text};
	REQUIRE((action == "shortage" || action == "deliver" || action == "inspect")); // A shortage orders only half the Steel; delivery buys only the missing original total; inspection performs no command or frame.
	const auto paths = GetTestCaseArtifactPaths("geothermal", "first-plant");
	VehicleID carrier = VehicleID::Invalid(); StationID origin = StationID::Invalid();
	bool started = false, completed = false, outbound = false, partial = false, empty_returned = false; std::exception_ptr failure;
	uint frames = 0; std::array<bool, 4> layers{}; uint64_t geology = 0;
	Money initial_cash = 0, purchased_cost = 0; std::array<Expenses, 3> initial_expenses{}; int initial_year = 0;
	FirstPlantDeliveryObservation initial;
	/** Actual afterload supplies only borrowed native identities and public commands, never a fixture world or an arrival. */
	std::function<void()> begin = [&] {
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; REQUIRE_FALSE(_pause_mode.Test(PauseMode::Error));
			REQUIRE(CaptureTBMProjects().empty()); REQUIRE(CaptureTBMSupport().empty()); REQUIRE(CaptureTBMExchangeAssignments().empty());
			// Both ordinary locomotives use model4. The new carrier owns three
			// manual stops; native automatically recorded pass-through visits may
			// enlarge its list without changing those original travel obligations.
			for (Train *train : Train::Iterate()) {
				if (!train->IsFrontEngine() || train->engine_type != GetUndergroundEngine(4) || train->GetNumManualOrders() != 3) continue;
				REQUIRE(carrier == VehicleID::Invalid()); carrier = train->index;
			}
			REQUIRE(carrier != VehicleID::Invalid()); const auto *head = Train::Get(carrier);
			std::vector<StationID> stops;
			// Borrow only the original manual sequence. Native implicit visits
			// remain saved and untouched, rather than being deleted to fit a test.
			for (const Order &order : head->Orders()) {
				if (order.IsType(OT_IMPLICIT)) continue;
				REQUIRE(order.IsType(OT_GOTO_STATION)); stops.push_back(order.GetDestination().ToStationID());
			}
			REQUIRE(stops.size() == 3); origin = stops[0]; REQUIRE(GetTileLayer(Station::Get(origin)->xy) == MapLayer::Surface);
			CHECK(GetTileLayer(Station::Get(stops[1])->xy) == MapLayer::Underground);
			CHECK(GetTileLayer(Station::Get(stops[2])->xy) == MapLayer::DeepUnderground);
			initial = ObserveFirstPlantMaterials(origin); PrintFirstPlantMaterials(initial);
			geology = ComputeGeologyStateDigest(GetGeologyWorldState()); const auto sources = ComputeSurfaceSupplyDigest();
			initial_cash = Company::Get(_current_company)->money; initial_expenses = Company::Get(_current_company)->yearly_expenses; initial_year = TimerGameEconomy::year.base();
			REQUIRE(SaveOrLoad(input, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology); CHECK(ComputeSurfaceSupplyDigest() == sources); CHECK(Company::Get(_current_company)->money == initial_cash);
			const auto inspected = paths.Output("saves", "first-plant-inspect.sav"); SaveFirstPlantDelivery(inspected);
			fmt::print("FIRST_PLANT_READ action=inspect cash={} path={}\n", initial_cash.base(), inspected.string());
			if (action == "inspect") { completed = true; _exit_game = true; return; }
			const auto target = action == "shortage" ? std::array<uint32_t, 4>{14, 0, 0, 0} : initial.required;
			// Retained monotonic paid history makes restarted commands purchase
			// only missing units; existing stock/dispatch never creates another bill.
			for (size_t column = 0; column < target.size(); ++column) {
				REQUIRE(initial.purchased[column] <= target[column]);
				const auto quantity = target[column] - initial.purchased[column]; if (quantity == 0) continue;
				const auto label = _first_plant_materials[column]; const auto quote = PrepareSurfaceSupplyOrder(origin, label, static_cast<uint16_t>(quantity));
				INFO("paid first-plant supply error=" << to_underlying(quote.error)); REQUIRE(quote.error == SurfaceSupplyError::None);
				const auto cash = Company::Get(_current_company)->money;
				REQUIRE(Command<Commands::OrderSurfaceSupply>::Do(DoCommandFlag::Execute, origin, label, static_cast<uint16_t>(quantity), quote.next_order).Succeeded());
				CHECK(Company::Get(_current_company)->money == cash - quote.cost); purchased_cost += quote.cost;
				fmt::print("FIRST_PLANT_DELIVERY_PURCHASE label={} units={} cost={} order={}\n", std::string(label.begin(), label.end()), quantity, quote.cost.base(), quote.next_order.base());
			}
			ObserveFirstPlantMaterials(origin);
			const auto purchased = paths.Output("saves", "first-plant-construction-purchased.sav"); SaveFirstPlantDelivery(purchased);
			fmt::print("FIRST_PLANT_DELIVERY_SAVE stage=purchased cash={} path={}\n", Company::Get(_current_company)->money.base(), purchased.string());
			if (head->vehstatus.Test(VehState::Stopped)) REQUIRE(Command<Commands::StartStopVehicle>::Do(DoCommandFlag::Execute, carrier, true).Succeeded());
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, false).Succeeded()); started = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	};
	/** @param elapsed Actual normal game frames; every arrival, payment and work increment belongs to the common game driver. */
	const IntervalTimer<TimerGameTick> timer({TimerGameTick::Priority::None, 1}, [&](uint elapsed) {
		if (!started) return;
		try {
			AutoRestoreBackup actor{_current_company, _local_company}; frames += elapsed;
			const auto *head = Train::Get(carrier); REQUIRE_FALSE(head->vehstatus.Test(VehState::Crashed));
			layers[to_underlying(GetTileLayer(head->tile))] = true;
			const auto state = ObserveFirstPlantMaterials(origin);
			uint32_t aboard = 0; for (uint32_t units : state.vehicles) aboard += units;
			// Retain a genuine loaded departure on Surface before either shaft,
			// including actual paid purpose and first-loading packet history.
			if (!outbound && aboard != 0 && GetTileLayer(head->tile) == MapLayer::Surface && !head->current_order.IsType(OT_LOADING)) {
				for (const Train *part = head; part != nullptr; part = part->Next()) {
					for (const CargoPacket *packet : *part->cargo.Packets()) {
						CHECK(packet->GetCompanySupplyOrder() != 0); CHECK(packet->HasCompanySupplyShipped()); CHECK(packet->GetFirstStation() == origin);
					}
				}
				const auto saved = paths.Output("saves", "first-plant-construction-outbound.sav"); SaveFirstPlantDelivery(saved);
				fmt::print("FIRST_PLANT_DELIVERY_SAVE stage=outbound cash={} path={}\n", Company::Get(_current_company)->money.base(), saved.string()); outbound = true;
			}
			const auto generator = CaptureGeothermalGenerators().front();
			// Save actual partially earned labour, without manually scheduling an
			// industry tick, supplying stock or overriding its retained progress.
			if (!partial && generator.work_ticks > 0 && generator.work_ticks < GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Powerhouse)->work_ticks) {
				const auto saved = paths.Output("saves", "first-plant-construction-partial.sav"); SaveFirstPlantDelivery(saved);
				fmt::print("FIRST_PLANT_DELIVERY_SAVE stage=partial cash={} path={}\n", Company::Get(_current_company)->money.base(), saved.string()); partial = true;
			}
			const bool shortage = state.purchased == std::array<uint32_t, 4>{14, 0, 0, 0} && state.waiting == state.purchased && aboard == 0 && CargoPacket::GetNumItems() == 0;
			// A complete full bill also retains a real empty arrival at the
			// original Surface station, after all purchased packets left the train.
			// Loading an empty source before dispatch cannot satisfy this return.
			if (state.dispatched == state.required && aboard == 0 && head->current_order.IsType(OT_LOADING) && head->last_station_visited == origin && GetTileLayer(head->tile) == MapLayer::Surface) empty_returned = true;
			const bool done = action == "shortage" ? shortage : (state.complete && empty_returned);
			if (!done && frames < 8000) return; // A bounded process may retain useful unfinished delivery/work, but cannot claim completion from elapsed time alone.
			INFO("original carrier " << DescribeTrain(head));
			if (!done) CHECK((state.dispatched != initial.dispatched || state.work > initial.work || state.waiting != initial.waiting)); // Actual first dispatch, accepted delivery or native labour must advance; an empty travel loop cannot certify progress.
			if (shortage) CHECK(state.work == 0);
			if (state.complete) { CHECK((state.consumed == state.required)); CHECK(aboard == 0); CHECK(CargoPacket::GetNumItems() == 0); }
			// Native vehicle profit includes real running costs. Nonpositive
			// profit is expected; the company revenue checks below prove that
			// company-supply freight earns no payment rather than free operation.
			CHECK(head->profit_this_year <= Money{0}); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
			const auto *company = Company::Get(_current_company); Money spent = 0;
			const int years = TimerGameEconomy::year.base() - initial_year; REQUIRE((years == 0 || years == 1)); // An8,000-frame burst can rotate one annual expense bucket, whose original paid cash must remain accounted.
			// Reconcile actual command, train and infrastructure expenses over all
			// retained buckets, including a bucket dropped by native year rotation.
			for (size_t year = 0; year < initial_expenses.size(); ++year) {
				for (size_t category = 0; category < initial_expenses[year].size(); ++category) {
					const auto kind = static_cast<ExpensesType>(category); spent += company->yearly_expenses[year][kind] - initial_expenses[year][kind];
					if (years == 1 && year == 2) spent += initial_expenses[year][kind]; // A real rotation cannot erase previous cash from the comparison.
				}
				CHECK(company->yearly_expenses[year][ExpensesType::TrainRevenue] == Money{0});
			}
			CHECK(company->money == initial_cash - spent); CHECK(spent >= purchased_cost); PrintFirstPlantMaterials(state);
			REQUIRE(Command<Commands::Pause>::Do(DoCommandFlag::Execute, PauseMode::Normal, true).Succeeded());
			const auto stage = done ? (action == "shortage" ? "shortage" : "complete") : "continued";
			const auto saved = paths.Output("saves", fmt::format("first-plant-construction-{}.sav", stage)); SaveFirstPlantDelivery(saved);
			fmt::print("FIRST_PLANT_DELIVERY_SAVE stage={} cash={} path={}\n", stage, company->money.base(), saved.string());
			fmt::print("FIRST_PLANT_DELIVERY_READY frames={} done={} work={} bought={} spent={} cash={} carrier={} surface={} underground={} deep={} empty_returned={}\n", frames, done ? 1 : 0,
				state.work, purchased_cost.base(), spent.base(), company->money.base(), carrier.base(), layers[to_underlying(MapLayer::Surface)] ? 1 : 0,
				layers[to_underlying(MapLayer::Underground)] ? 1 : 0, layers[to_underlying(MapLayer::DeepUnderground)] ? 1 : 0, empty_returned ? 1 : 0);
			completed = true; started = false; _exit_game = true;
		} catch (...) { failure = std::current_exception(); started = false; _exit_game = true; }
	});
	AutoRestoreBackup observer{_first_plant_delivery_observer, &begin}; IConsole::CmdRegister("first_plant_deliver", ObserveFirstPlantDelivery);
	const std::string config = GetTestRuntimeConfig().string();
	std::array<std::string_view, 17> arguments{"openttd_test.exe", "-v", "null:ticks=8005", "-b", "8bpp-simple", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config, "-g", input};
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); CHECK(completed);
}
