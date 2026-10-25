/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_useful_fixture.h Full ordinary native operation of the actual earned-floor modal route.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6DCDD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_NORMAL_GROUND_USEFUL_FIXTURE_H
#define TESTS_NORMAL_GROUND_USEFUL_FIXTURE_H

#include "normal_ground_runtime_fixture.h"
#include "normal_ground_video_fixture.h"
#include "../surface_supply_cmd.h"
#include "../surface_supply_order_runtime.h"

/** @param name Explicit controller value bound to the exact input receipt. @return Checked numeric observation selector/budget. */
inline uint32_t NormalGroundUsefulArgument(const char *name)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr); uint32_t value = 0;
	const std::string_view argument{text}; const auto parsed = std::from_chars(argument.data(), argument.data() + argument.size(), value);
	REQUIRE(parsed.ec == std::errc{}); REQUIRE(parsed.ptr == argument.data() + argument.size()); return value;
}

/** @param front Actual ordered front. @param index Actual retained depot order. @return Its native public halt flag. */
inline bool NormalGroundUsefulHalt(const Train &front, int index)
{
	const auto *order = front.GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_DEPOT));
	return order->GetDepotActionType().Test(OrderDepotActionFlag::Halt);
}

/** Observe physical useful packets without producing or transferring stock.
 * @param tick Actual native tick.
 * @param carrier Original paid modal carrier.
 * @param capsule Original paid tube front.
 */
inline void PrintNormalGroundUsefulPackets(uint64_t tick, const Train &carrier, const Train &capsule)
{
	// Follow both real consists and their native packet lists; repeated custody
	// views must retain the original industry source and cannot create stock.
	for (const auto *front : {&carrier, &capsule}) for (const auto *part = front; part != nullptr; part = part->Next()) {
		for (const CargoPacket *packet : *part->cargo.Packets()) {
			CHECK(packet->Count() > 0); CHECK(packet->GetSource().type == SourceType::Industry); CHECK(packet->GetSource().id == 0);
			CHECK(packet->GetFirstStation() == StationID{0}); CHECK(GetTileLayer(packet->GetSourceTile()) == MapLayer::Surface);
			fmt::print("NORMAL_GROUND_USEFUL_PACKET tick={} front={} part={} tile={} packet={} amount={} age={} feeder={} first={} source={} source_kind={} source_id={}\n",
				tick, front->index.base(), part->index.base(), part->tile.base(), packet->index.base(), packet->Count(), packet->GetPeriodsInTransit(), packet->GetFeederShare().base(),
				packet->GetFirstStation().base(), packet->GetSourceTile().base(), to_underlying(packet->GetSource().type), packet->GetSource().id);
		}
	}
}

/** Full GameLoop opportunities own actual supply,curved tube motion,modal transfer,Surface payment and empty depot returns. */
inline void RunNormalGroundUsefulOperation()
{
	const char *input = std::getenv("OTTD_NORMAL_GROUND_INPUT"); REQUIRE(input != nullptr);
	const auto carrier_id = VehicleID{NormalGroundUsefulArgument("OTTD_NORMAL_GROUND_CARRIER")};
	const auto capsule_id = VehicleID{NormalGroundUsefulArgument("OTTD_NORMAL_GROUND_CAPSULE")};
	const auto budget = NormalGroundUsefulArgument("OTTD_NORMAL_GROUND_TICKS"); REQUIRE(budget > 0); REQUIRE(budget <= 32768);
	bool begin_requested = false, started = false, requested = false, finished = false;
	std::exception_ptr failure; CompanyID owner = CompanyID::Invalid(); CargoType coal = INVALID_CARGO;
	std::unique_ptr<NormalFreightEarnings> accounting;
	std::optional<decltype(ComputeGeologyStateDigest(GetGeologyWorldState()))> geology;
	uint64_t first_tick = 0, first_energy = 0, first_invoice = 0, last_trace = 0;
	Money opening = 0; UtilityProducerID source{UINT32_MAX};
	SurfaceSupplyOrderID native_supply = 0; ///< One new canonical purchase, separate from original industry Coal.
	uint64_t peak_carrier = 0, peak_capsule = 0; std::set<TileIndex> carrier_visits, capsule_visits;
	bool surface_pickup = false, loaded_curve = false, loaded_ug_return = false, loaded_surface_return = false;
	using Trace = std::tuple<TileIndex, uint, uint64_t, TileIndex, uint, uint64_t, uint, uint, uint64_t>;
	std::optional<Trace> previous;
	std::function<void()> begin = [&]() { begin_requested = true; };
	AutoRestoreBackup hook{_normal_ground_newgame_observer, &begin};
	IConsole::CmdRegister("normal_ground_useful", ObserveNormalGroundNewgame);
	/** Observe complete prior-frame owners;never advance a private tick,vehicle,packet or utility dispatcher. */
	const auto observe = [&]() {
		accounting->Observe(); const auto *carrier = Train::Get(carrier_id), *capsule = Train::Get(capsule_id);
		const auto *wagon = carrier->Next(); REQUIRE(wagon != nullptr); REQUIRE(wagon->Next() == nullptr);
		const auto carrier_held = wagon->cargo.StoredCount(), capsule_held = capsule->cargo.StoredCount();
		carrier_visits.insert(carrier->tile); capsule_visits.insert(capsule->tile);
		peak_carrier = std::max<uint64_t>(peak_carrier, carrier_held); peak_capsule = std::max<uint64_t>(peak_capsule, capsule_held);
		// The Surface pickup milestone requires actual cargo after the named
		// original source visit; merely passing its tile does not prove supply.
		if (carrier_held > 0 && carrier->last_station_visited == StationID{0} && GetTileLayer(wagon->tile) == MapLayer::Surface) surface_pickup = true;
		// A loaded moving capsule on the real curve proves useful tube routing,
		// independently of an empty vehicle animation or declared route.
		if (capsule_held > 0 && capsule->tile == TileXYL(96, 69, MapLayer::Underground) && capsule->cur_speed > 0) loaded_curve = true;
		// The named receiving station must supply the retained return leg;
		// observe its load through both physical layers without teleportation.
		if (carrier_held > 0 && carrier->last_station_visited == StationID{12}) {
			loaded_ug_return |= GetTileLayer(wagon->tile) == MapLayer::Underground;
			loaded_surface_return |= GetTileLayer(wagon->tile) == MapLayer::Surface;
		}
		const auto a_stock = Station::Get(StationID{11})->goods[coal].TotalCount(), b_stock = Station::Get(StationID{12})->goods[coal].TotalCount();
		const auto delivered = accounting->delivered_by_cargo[coal];
		const Trace trace{carrier->tile, carrier->cur_real_order_index, carrier_held, capsule->tile, capsule->cur_real_order_index, capsule_held, a_stock, b_stock, delivered};
		// Capture real route/stock changes and a bounded wait heartbeat; no presentation clock drives transport.
		if (!previous || trace != *previous || TimerGameTick::counter - last_trace >= 512) {
			fmt::print("NORMAL_GROUND_USEFUL_TRACE tick={} carrier_tile={} carrier_order={} carrier_station={} carrier_held={} capsule_tile={} capsule_order={} capsule_station={} capsule_held={} a_stock={} b_stock={} delivered={} energy={} invoice={} cash={}\n",
				TimerGameTick::counter, carrier->tile.base(), carrier->cur_real_order_index, carrier->last_station_visited.base(), carrier_held,
				capsule->tile.base(), capsule->cur_real_order_index, capsule->last_station_visited.base(), capsule_held, a_stock, b_stock, delivered,
				GetUtilityPowerProducer(source)->consumed_lifetime_j, GetTubePowerConnection(source)->invoiced_money, Company::Get(owner)->money.base());
			PrintNormalGroundUsefulPackets(TimerGameTick::counter, *carrier, *capsule);
			previous = trace; last_trace = TimerGameTick::counter; std::fflush(stdout);
		}
		const bool returned = NormalGroundUsefulHalt(*carrier, 0) && NormalGroundUsefulHalt(*capsule, 3) &&
			carrier->IsStoppedInDepot() && carrier->tile == TileXYL(29, 40, MapLayer::Surface) &&
			capsule->IsStoppedInDepot() && capsule->tile == TileXYL(96, 66, MapLayer::Underground) &&
			carrier_held == 0 && capsule_held == 0 && a_stock == 0 && b_stock == 0 && carrier->cargo_payment == nullptr;
		// Return requires native empty depots and settled cargo; a budget limit retains an explicitly unfinished save.
		if (returned || TimerGameTick::counter - first_tick >= budget) requested = true;
	};
	const IntervalTimer<TimerGameTick> observer({TimerGameTick::Priority::None, 1}, [&](uint) {
		// A realtime saver can run a few complete frames after the requested
		// budget. Observe those actual frames as well;never trim their owners.
		// Observe successful running segments only; stop does not grant a private simulation opportunity.
		if (!started || finished || failure) return;
		try { observe(); } catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	const IntervalTimer<TimerGameRealtime> controls({std::chrono::milliseconds(1), TimerGameRealtime::Trigger::Always}, [&](auto) {
		// Startup, shutdown and failure are outside paid player-control opportunities.
		if (!begin_requested || finished || failure) return;
		try {
			if (!started) {
				auto pending = _pause_mode; pending.Reset(PauseMode::Normal); pending.Reset(PauseMode::CommandDuringPause); if (pending.Any()) return;
				owner = _local_company; AutoRestoreBackup actor{_current_company, owner};
				coal = GetCargoTypeByLabel(CT_COAL); REQUIRE(IsValidCargoType(coal));
				auto *carrier = Train::Get(carrier_id), *capsule = Train::Get(capsule_id);
				REQUIRE(carrier->owner == owner); REQUIRE(capsule->owner == owner);
				REQUIRE(carrier->engine_type == GetUndergroundEngine(4)); REQUIRE(carrier->Next()->engine_type == GetUndergroundEngine(5)); REQUIRE(capsule->engine_type == GetTubeEngine(9));
				REQUIRE(Train::Get(VehicleID{2})->IsStoppedInDepot()); REQUIRE(Train::Get(VehicleID{33})->IsStoppedInDepot());
				const auto connections = GetTubePowerConnections(); REQUIRE(connections.size() == 1); source = connections.front().producer;
				first_energy = GetUtilityPowerProducer(source)->consumed_lifetime_j; first_invoice = connections.front().invoiced_money;
				opening = Company::Get(owner)->money; first_tick = TimerGameTick::counter;
				accounting = std::make_unique<NormalFreightEarnings>(*Company::Get(owner)); geology = ComputeGeologyStateDigest(GetGeologyWorldState());
				// Exercise the merged Pending-only purchase owner in this genuinely
				// financed world. Steel stays separate from naturally produced Coal;
				// the production station scheduler alone may create the paid stock.
				native_supply = CaptureSurfaceSupplyOrders().next_id;
				constexpr uint32_t steel_wire = 0x5354454C; // STEL's explicit big-endian synchronized command wire.
				PayNormalFreightCommand<Commands::PlaceSurfaceSupplyOrder>("merged_native_steel", StationID{0}, steel_wire, uint16_t{8}, native_supply);
				PayNormalFreightCommand<Commands::PlaceSurfaceSupplyOrder>("merged_native_steel_retry", StationID{0}, steel_wire, uint16_t{8}, native_supply);
				if (_pause_mode.Test(PauseMode::Normal)) REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, false));
				for (auto *front : {carrier, capsule}) {
					// This route deliberately passes the old Surface consumer and
					// portal yard before its named UG destination. Ordinary default
					// intermediate stops would deliver Coal before the modal leg.
					// Use the same public non-stop control available to the player.
					// Apply the ordinary non-stop option to every retained order so only the named useful destination receives cargo.
					for (uint index = 0; index < front->GetNumOrders(); ++index) {
						const auto *order = front->GetOrder(index);
						if (!order->GetNonStopType().Test(OrderNonStopFlag::NonStop))
							PayNormalFreightCommand<Commands::ModifyOrder>("useful_skip_intermediate", front->tile, front->index, static_cast<VehicleOrderID>(index), MOF_NON_STOP, OrderNonStopFlags{OrderNonStopFlag::NonStop}.base());
					}
					const int home_order = front == carrier ? 0 : 3;
					// Restart only a stopped original service whose retained order
					// still requests work; preserve an explicit player depot halt.
					if (front->vehstatus.Test(VehState::Stopped) && !NormalGroundUsefulHalt(*front, home_order))
						PayNormalFreightCommand<Commands::StartStopVehicle>("start_useful_front", front->tile, front->index, true);
				}
				started = true; return;
			}
			AutoRestoreBackup actor{_current_company, owner};
			auto *carrier = Train::Get(carrier_id), *capsule = Train::Get(capsule_id);
			if (!requested) {
				// Order an empty capsule return only after the physical modal
				// carrier holds the full destination load, so no stock is stranded.
				if (!NormalGroundUsefulHalt(*capsule, 3) && carrier->last_station_visited == StationID{12} && carrier->Next()->cargo.StoredCount() == carrier->Next()->cargo_cap) {
					PayNormalFreightCommand<Commands::ModifyOrder>("useful_capsule_empty_home", capsule->tile, capsule_id, VehicleOrderID{3}, MOF_DEPOT_ACTION, static_cast<uint16_t>(OrderDepotAction::Stop));
					fmt::print("NORMAL_GROUND_USEFUL_CONTROL tick={} action=1\n", TimerGameTick::counter);
				}
				// The same completed real transfer permits the carrier's final
				// depot halt, preserving its loaded return and ordinary payment.
				if (!NormalGroundUsefulHalt(*carrier, 0) && carrier->last_station_visited == StationID{12} && carrier->Next()->cargo.StoredCount() == carrier->Next()->cargo_cap) {
					PayNormalFreightCommand<Commands::ModifyOrder>("useful_carrier_empty_home", carrier->tile, carrier_id, VehicleOrderID{0}, MOF_DEPOT_ACTION, static_cast<uint16_t>(OrderDepotAction::Stop));
					fmt::print("NORMAL_GROUND_USEFUL_CONTROL tick={} action=2\n", TimerGameTick::counter);
				}
				return;
			}
			observe(); REQUIRE(Command<Commands::Pause>::Post(PauseMode::Normal, true));
			const auto *company = Company::Get(owner); Money total = 0;
			// Reconcile all native expense categories rather than inferring settlement from income alone.
			for (uint index = 0; index < to_underlying(ExpensesType::End); ++index) {
				const auto cost = accounting->ledger[static_cast<ExpensesType>(index)]; total += cost;
				fmt::print("NORMAL_GROUND_USEFUL_LEDGER category={} cost={}\n", index, cost.base());
			}
			CHECK(company->money == opening - total); CHECK(company->current_loan == Money{300000});
			const auto *purchase = GetSurfaceSupplyOrder(native_supply);
			REQUIRE(purchase != nullptr); REQUIRE(purchase->Native() != nullptr);
			CHECK(purchase->quantity == 8); CHECK(purchase->origin == StationID{0});
			CHECK(purchase->Native()->state == SurfaceSupplyOrderState::Fulfilled);
			CHECK(Station::Get(StationID{0})->goods[GetCargoTypeByLabel(CargoLabel{"STEL"})].TotalCount() >= 8);
			CHECK(CmdCancelSurfaceSupplyOrder({}, native_supply).Failed());
			fmt::print("NORMAL_GROUND_CANONICAL_SUPPLY intent={} quantity={} cost={} fulfilled=1 refund_refused=1\n",
				native_supply, purchase->quantity, purchase->Native()->cost.base());
			CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == *geology); CHECK(_settings_game.pf.forbid_90_deg);
			const auto a_stock = Station::Get(StationID{11})->goods[coal].TotalCount(), b_stock = Station::Get(StationID{12})->goods[coal].TotalCount();
			const bool returned = NormalGroundUsefulHalt(*carrier, 0) && NormalGroundUsefulHalt(*capsule, 3) &&
				carrier->IsStoppedInDepot() && carrier->tile == TileXYL(29, 40, MapLayer::Surface) && capsule->IsStoppedInDepot() && capsule->tile == TileXYL(96, 66, MapLayer::Underground) &&
				carrier->Next()->cargo.StoredCount() == 0 && capsule->cargo.StoredCount() == 0 && a_stock == 0 && b_stock == 0 && carrier->cargo_payment == nullptr;
			const auto paths = GetTestCaseArtifactPaths("acceptance", "normal-ground-useful");
			for (const auto name : {"useful-progress.sav", "useful-progress-repeat.sav"})
				REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(paths.Output("saves", name)), false) == SaveLoadResult::Ok);
			fmt::print("NORMAL_GROUND_USEFUL first_tick={} tick={} opening_cash={} cash={} loan={} carrier={} capsule={} peak_carrier={} peak_capsule={} carrier_visited={} capsule_visited={} surface_pickup={} loaded_curve={} loaded_ug_return={} loaded_surface_return={} delivered={} first_energy={} energy={} first_invoice={} invoice={} a_stock={} b_stock={} carrier_home={} capsule_home={} complete={}\n",
				first_tick, TimerGameTick::counter, opening.base(), company->money.base(), company->current_loan.base(), carrier_id.base(), capsule_id.base(), peak_carrier, peak_capsule, carrier_visits.size(), capsule_visits.size(),
				surface_pickup ? 1 : 0, loaded_curve ? 1 : 0, loaded_ug_return ? 1 : 0, loaded_surface_return ? 1 : 0, accounting->delivered_by_cargo[coal], first_energy,
				GetUtilityPowerProducer(source)->consumed_lifetime_j, first_invoice, GetTubePowerConnection(source)->invoiced_money, a_stock, b_stock,
				carrier->IsStoppedInDepot() && carrier->tile == TileXYL(29, 40, MapLayer::Surface) ? 1 : 0,
				capsule->IsStoppedInDepot() && capsule->tile == TileXYL(96, 66, MapLayer::Underground) ? 1 : 0, returned ? 1 : 0);
			std::fflush(stdout); finished = true; _exit_game = true;
		} catch (...) { failure = std::current_exception(); _exit_game = true; }
	});
	const std::string driver = "ground-acceptance-null";
	std::vector<std::string> owned = {"openttd_test.exe", "-x", "-X", "-c", GetTestRuntimeConfig().string(), "-v", driver, "-b", "8bpp-simple", "-r", "640x480", "-s", "null", "-m", "null", "-I", "original_dos", "-g", input};
	std::vector<std::string_view> arguments; for (const auto &value : owned) arguments.push_back(value);
	CHECK(openttd_main(arguments) == 0); if (failure) std::rethrow_exception(failure); REQUIRE(started); REQUIRE(finished);
}

#endif /* TESTS_NORMAL_GROUND_USEFUL_FIXTURE_H */
