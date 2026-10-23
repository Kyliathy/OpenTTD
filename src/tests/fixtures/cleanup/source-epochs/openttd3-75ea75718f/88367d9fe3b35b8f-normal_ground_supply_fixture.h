/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_supply_fixture.h Actual Surface supply,retained carriage and paid tube service on earned floor.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6DCDC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * This fixture loads an exact original;it never constructs a substitute world.
 */
#ifndef TESTS_NORMAL_GROUND_SUPPLY_FIXTURE_H
#define TESTS_NORMAL_GROUND_SUPPLY_FIXTURE_H

#include "normal_ground_freight_fixture.h"
#include "../tbm.h"
#include "../transport_tube.h"
#include "../underground_vehicle.h"
#include "../transport_tube_cmd.h"
#include "../utility_power.h"
#include "../depot_base.h"

/** @param depot Actual native depot. @param engine Native available definition. @param cargo Native cargo refit. @return Actually purchased new part. */
inline Train *BuyNormalGroundSupplyPart(TileIndex depot, EngineID engine, CargoType cargo)
{
	REQUIRE(IsEngineBuildable(engine, VehicleType::Train, _current_company));
	std::set<VehicleID> old; for (const auto *part : Train::Iterate()) old.insert(part->index);
	PayNormalFreightCommand<Commands::BuildVehicle>("modal_vehicle", depot, engine, false, cargo, ClientID::Invalid);
	Train *created = nullptr;
	for (auto *part : Train::Iterate()) if (!old.contains(part->index)) { REQUIRE(created == nullptr); created = part; }
	REQUIRE(created != nullptr); REQUIRE(created->engine_type == engine); return created;
}

/** @param station Actual same-layer station. @param transfer Transfer without final payment. @param full Wait for a real complete load. @return Native retained station order. */
inline Order NormalGroundSupplyStationOrder(StationID station, bool transfer, bool full = false)
{
	Order order; order.MakeGoToStation(station);
	order.SetUnloadType(transfer ? OrderUnloadType::Transfer : OrderUnloadType::NoUnload);
	order.SetLoadType(transfer ? OrderLoadType::NoLoad : full ? OrderLoadType::FullLoadAny : OrderLoadType::LoadIfPossible);
	return order;
}

/** @param depot Actual built depot. @return Ordinary ordered native visit/reversal,without a halt flag. */
inline Order NormalGroundSupplyDepotOrder(TileIndex depot)
{
	Order order; order.MakeGoToDepot(Depot::GetByTile(depot)->index, {OrderDepotTypeFlag::PartOfOrders}, OrderNonStopFlag::NonStop, {}); return order;
}

/** @param front Actual stopped front. @param orders Real native orders. Replace old service through public commands only. */
inline void SetNormalGroundSupplyOrders(Train &front, std::span<const Order> orders)
{
	REQUIRE(front.IsStoppedInDepot());
	while (front.GetNumOrders() != 0) PayNormalFreightCommand<Commands::DeleteOrder>("modal_old_order", front.tile, front.index, VehicleOrderID{0});
	for (size_t index = 0; index < orders.size(); ++index)
		PayNormalFreightCommand<Commands::InsertOrder>("modal_order", front.tile, front.index, static_cast<VehicleOrderID>(index), orders[index]);
}

/**
 * Pay for an actual Surface connection,real retained consist,native capsule and sole external power owner.
 * @param carrier Previously bought empty Deep hauler,actually parked in its Surface depot.
 * @return The newly bought capsule;both useful fronts remain stopped until the next physical operation producer.
 */
inline Train *PrepareNormalGroundUsefulSupply(Train &carrier)
{
	const auto surface = [](uint x, uint y) { return TileXYL(x, y, MapLayer::Surface); };
	const auto underground = [](uint x, uint y) { return TileXYL(x, y, MapLayer::Underground); };
	const auto coal_source = GetStationIndex(surface(29, 41)), coal_consumer = GetStationIndex(surface(69, 96));
	REQUIRE(coal_source == StationID{0}); REQUIRE(coal_consumer == StationID{1});
	const auto coal = GetCargoTypeByLabel(CT_COAL); REQUIRE(IsValidCargoType(coal));
	const auto origin = GetStationIndex(underground(96, 68)), destination = GetStationIndex(underground(101, 69));
	REQUIRE(origin == StationID{11}); REQUIRE(destination == StationID{12});
	const auto tube_home = underground(96, 66), tube_far = underground(103, 69), freight_far = underground(103, 70);
	const auto depot = surface(95, 57); REQUIRE(carrier.tile == depot); REQUIRE(carrier.IsStoppedInDepot());
	const auto *company = Company::Get(_current_company);
	const auto quote_before = std::tuple{company->money, TimerGameTick::counter, _random.state[0], _random.state[1], Vehicle::GetNumItems()};
	const auto proposal = SelectNormalFreightRails(surface(69, 96), surface(95, 61), true);
	Money track_cost = 0;
	for (const auto &piece : proposal) {
		if (IsPlainRailTile(piece.tile) && GetRailType(piece.tile) == RAILTYPE_RAIL && GetTrackBits(piece.tile).Test(piece.track)) continue;
		const auto quote = Command<Commands::BuildRail>::Do({}, piece.tile, RAILTYPE_RAIL, piece.track, false);
		REQUIRE(quote.Succeeded()); track_cost += quote.GetCost();
	}
	const auto capsule_quote = ExtractCommandCost(Command<Commands::BuildVehicle>::Do({}, tube_home, GetTubeEngine(9), false, coal, ClientID::Invalid));
	const auto wagon_quote = ExtractCommandCost(Command<Commands::BuildVehicle>::Do({}, depot, GetUndergroundEngine(5), false, coal, ClientID::Invalid));
	REQUIRE(capsule_quote.Succeeded()); REQUIRE(wagon_quote.Succeeded());
	const auto lease = GetMultiLayerTubePowerContract().lease_per_day;
	CHECK((std::tuple{company->money, TimerGameTick::counter, _random.state[0], _random.state[1], Vehicle::GetNumItems()} == quote_before));
	fmt::print("NORMAL_GROUND_SUPPLY_QUOTE pieces={} tracks={} capsule={} wagon={} lease={} cash={}\n", proposal.size(), track_cost.base(), capsule_quote.GetCost().base(), wagon_quote.GetCost().base(), lease, company->money.base());
	REQUIRE(company->money > track_cost + capsule_quote.GetCost() + wagon_quote.GetCost() + Money{lease});
	for (const auto &piece : proposal) {
		if (IsPlainRailTile(piece.tile) && GetRailType(piece.tile) == RAILTYPE_RAIL && GetTrackBits(piece.tile).Test(piece.track)) continue;
		PayNormalFreightCommand<Commands::BuildRail>("modal_surface_connection", piece.tile, RAILTYPE_RAIL, piece.track, false);
	}
	std::vector<VehicleID> stored;
	for (const auto *part = carrier.Next(); part != nullptr; part = part->Next()) {
		REQUIRE(part->cargo.TotalCount() == 0); stored.push_back(part->index);
	}
	REQUIRE(stored.size() == 6);
	PayNormalFreightCommand<Commands::MoveRailVehicle>("modal_store_spoil_chain", depot, stored.front(), VehicleID::Invalid(), true);
	REQUIRE(carrier.Next() == nullptr);
	auto *wagon = BuyNormalGroundSupplyPart(depot, GetUndergroundEngine(5), coal);
	if (wagon->First() != &carrier) PayNormalFreightCommand<Commands::MoveRailVehicle>("modal_resource_couple", depot, wagon->index, carrier.Last()->index, false);
	REQUIRE(carrier.Next() == wagon); REQUIRE(wagon->Next() == nullptr); REQUIRE(wagon->cargo_type == coal);
	auto *capsule = BuyNormalGroundSupplyPart(tube_home, GetTubeEngine(9), coal);
	const std::vector wire{underground(95, 68), underground(96, 68), underground(96, 67), tube_home};
	const auto physical_before = ComputeTubePowerDigest(), utility_before = ComputeUtilityPowerDigest();
	const auto power_quote = Command<Commands::ConnectTubePower>::Do({}, underground(95, 67), tube_home, TubePowerCommandPath{wire});
	REQUIRE(power_quote.Succeeded()); CHECK(power_quote.GetCost() == Money{0});
	CHECK(ComputeTubePowerDigest() == physical_before); CHECK(ComputeUtilityPowerDigest() == utility_before);
	const auto cash_before = company->money;
	REQUIRE(Command<Commands::ConnectTubePower>::Post(underground(95, 67), tube_home, TubePowerCommandPath{wire}));
	const auto connections = GetTubePowerConnections(); REQUIRE(connections.size() == 1);
	const auto &source = connections.front(); REQUIRE(source.invoiced_money == lease);
	CHECK(cash_before - company->money == Money{static_cast<int64_t>(source.invoiced_money)});
	// The physical source already debits its sole lease;record that invoice
	// once rather than calling the generic zero-CommandCost cash reconciler.
	fmt::print("NORMAL_FREIGHT_INVOICE action=modal_external_lease cost={} cash={}\n", source.invoiced_money, company->money.base());
	PayNormalFreightCommand<Commands::SetTubeVehiclePower>("modal_bind_power", capsule->index, source.producer, false);
	const std::array tube_orders{NormalGroundSupplyStationOrder(origin, false), NormalGroundSupplyStationOrder(destination, true),
		NormalGroundSupplyDepotOrder(tube_far), NormalGroundSupplyDepotOrder(tube_home)};
	SetNormalGroundSupplyOrders(*capsule, tube_orders);
	Order final; final.MakeGoToStation(coal_consumer); final.SetUnloadType(OrderUnloadType::UnloadIfPossible); final.SetLoadType(OrderLoadType::NoLoad);
	const std::array freight_orders{NormalGroundSupplyDepotOrder(surface(29, 40)), NormalGroundSupplyStationOrder(coal_source, false, true),
		NormalGroundSupplyStationOrder(origin, true), NormalGroundSupplyDepotOrder(freight_far), NormalGroundSupplyStationOrder(destination, false, true), final};
	SetNormalGroundSupplyOrders(carrier, freight_orders);
	for (const auto id : stored) {
		const auto *part = Train::Get(id); REQUIRE(part->tile == depot); REQUIRE(part->cargo.TotalCount() == 0); REQUIRE(part->First() != &carrier);
		fmt::print("NORMAL_GROUND_SUPPLY_STORED vehicle={} front={} tile={} cargo={} count={}\n", id.base(), part->First()->index.base(), part->tile.base(), part->cargo_type, part->cargo.TotalCount());
	}
	fmt::print("NORMAL_GROUND_SUPPLY_MACHINERY carrier={} wagon={} capsule={} source={} cargo={} capacity={} origin={} destination={} coal_source={} coal_consumer={} orders={} tube_orders={} lease={}\n",
		carrier.index.base(), wagon->index.base(), capsule->index.base(), source.producer.base(), coal, wagon->cargo_cap, origin.base(), destination.base(), coal_source.base(), coal_consumer.base(), carrier.GetNumOrders(), capsule->GetNumOrders(), source.invoiced_money);
	CHECK(carrier.IsStoppedInDepot()); CHECK(capsule->IsStoppedInDepot());
	CHECK(GetUtilityPowerProducer(source.producer)->consumed_lifetime_j == 0);
	return capsule;
}

/** Clear the actual shared lines through ordinary full-frame depot travel,then prepare stopped useful participants. */
inline void RunNormalGroundUsefulSupply()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	constexpr uint32_t budget = 8192;
	bool begin_requested = false, started = false, requested = false, finished = false;
	std::exception_ptr failure; CompanyID owner = CompanyID::Invalid(); VehicleID carrier_id = VehicleID::Invalid();
	std::unique_ptr<NormalFreightEarnings> accounting;
	std::optional<decltype(ComputeGeologyStateDigest(GetGeologyWorldState()))> geology;
	uint64_t first_tick = 0; Money opening = 0;
	const std::array parking{VehicleID{2}, VehicleID{33}};
	std::map<VehicleID, std::set<TileIndex>> visits;
	std::function<void()> begin = [&]() { begin_requested = true; };
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &begin};
	IConsole::CmdRegister("normal_ground_supply", ObserveNormalGroundNewgame);
	const IntervalTimer<TimerGameTick> observer({TimerGameTick::Priority::None, 1}, [&](uint) {
		if (!started || requested || failure) return;
		try {
			accounting->Observe(); bool parked = true;
			for (const auto id : parking) { const auto *front = Train::Get(id); visits[id].insert(front->tile); parked &= front->IsStoppedInDepot(); }
			requested = parked || TimerGameTick::counter - first_tick >= budget;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	const IntervalTimer<TimerGameRealtime> controls({std::chrono::milliseconds(1), TimerGameRealtime::Trigger::Always}, [&](auto) {
		if (!begin_requested || finished || failure) return;
		try {
			if (!started) {
				auto pending = _pause_mode; pending.Reset(PauseMode::Normal); pending.Reset(PauseMode::CommandDuringPause); if (pending.Any()) return;
				owner = _local_company; REQUIRE(Company::IsValidID(owner)); AutoRestoreBackup actor{_current_company, owner};
				const auto *company = Company::Get(owner); opening = company->money; first_tick = TimerGameTick::counter;
				geology = ComputeGeologyStateDigest(GetGeologyWorldState()); accounting = std::make_unique<NormalFreightEarnings>(*company);
				REQUIRE(CaptureTBMProjects().empty()); REQUIRE(CaptureTBMSupport().empty());
				for (const auto *front : Train::Iterate()) if (front->IsFrontEngine() && front->owner == owner && front->engine_type == GetUndergroundEngine(4) &&
					front->IsStoppedInDepot() && front->tile == TileXYL(95, 57, MapLayer::Surface)) { REQUIRE(carrier_id == VehicleID::Invalid()); carrier_id = front->index; }
				REQUIRE(carrier_id != VehicleID::Invalid());
				if (_pause_mode.Test(PauseMode::Normal)) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
				for (const auto id : parking) {
					auto *front = Train::Get(id); REQUIRE(front->owner == owner); visits.emplace(id, std::set<TileIndex>{front->tile});
					if (!front->IsStoppedInDepot()) {
						if (!front->current_order.IsType(OT_GOTO_DEPOT)) PayNormalFreightCommand<Commands::SendVehicleToDepot>("modal_park_old_service", id, DepotCommandFlags{}, VehicleListIdentifier{});
						if (front->vehstatus.Test(VehState::Stopped)) PayNormalFreightCommand<Commands::StartStopVehicle>("modal_start_depot_trip", front->tile, id, true);
					}
				}
				started = true; return;
			}
			if (!requested) return;
			AutoRestoreBackup actor{_current_company, owner};
			for (const auto id : parking) {
				const auto *front = Train::Get(id); REQUIRE(front->IsStoppedInDepot());
				fmt::print("NORMAL_GROUND_SUPPLY_PARKED vehicle={} tile={} visited={} stored=1\n", id.base(), front->tile.base(), visits.at(id).size());
			}
			auto *capsule = PrepareNormalGroundUsefulSupply(*Train::Get(carrier_id));
			accounting->Observe(); REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
			const auto *company = Company::Get(owner); Money total = 0;
			for (uint index = 0; index < to_underlying(ExpensesType::End); ++index) {
				const auto cost = accounting->ledger[static_cast<ExpensesType>(index)]; total += cost;
				fmt::print("NORMAL_GROUND_SUPPLY_LEDGER category={} cost={}\n", index, cost.base());
			}
			CHECK(company->money == opening - total); CHECK(company->current_loan == Money{300000});
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == *geology); CHECK(_settings_game.pf.forbid_90_deg);
			const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-supply");
			for (const auto name : {"useful-ready.sav", "useful-ready-repeat.sav"})
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
			fmt::print("NORMAL_GROUND_SUPPLY_READY first_tick={} tick={} opening_cash={} cash={} loan={} carrier={} capsule={} complete=1\n",
				first_tick, TimerGameTick::counter, opening.base(), company->money.base(), company->current_loan.base(), carrier_id.base(), capsule->index.base());
			std::fflush(stdout); finished = true; _exit_game = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	const auto driver = fmt::format("null:ticks={}", budget + 3000);
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(), "-v", driver, "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null", "-I", "original_dos", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); REQUIRE(started); REQUIRE(finished);
}

#endif /* TESTS_NORMAL_GROUND_SUPPLY_FIXTURE_H */
