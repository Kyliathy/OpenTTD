/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_freight_efficiency.h Pure original freight timeliness and available-load admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSB.
 * Existing coupled wagons, cargo, native orders and finances retain ownership.
 * A quoted pickup-policy change predicts neither extra cargo nor future fares.
 */
#ifndef TEST_SURFACE_ENTRY_FREIGHT_EFFICIENCY_H
#define TEST_SURFACE_ENTRY_FREIGHT_EFFICIENCY_H

/** Characterize actual train geometry and price one zero-purchase available-load proposal. */
static void QuerySurfaceFreightEfficiency()
{
	const auto *company = Company::Get(_current_company); REQUIRE(company->money == Money{80157});
	uint heads = 0, total_parts = 0;
	// Every original freight role retains its complete paid consist and route.
	// Only the future pickup wait policy is proposed; all current cargo stays put.
	for (const auto *train : Train::Iterate()) {
		if (!train->IsFrontEngine()) continue;
		REQUIRE(train->owner == _current_company); REQUIRE(train->name.starts_with("Surface Income "));
		++heads; uint parts = 0; CargoArray capacity{}, held{};
		for (const Train *part = train; part != nullptr; part = part->Next()) {
			++parts; capacity[part->cargo_type] += part->cargo_cap; held[part->cargo_type] += part->cargo.TotalCount();
		}
		total_parts += parts; REQUIRE(train->GetNumOrders() == 2);
		fmt::print("SURFACE_RESUPPLY_THROUGHPUT_HEAD vehicle={} engine={} parts={} length={} weight={} power={} curve_limit={} display_max_speed={} acceleration_model={} remaining={}\n",
			train->index.base(), train->engine_type.base(), parts, train->gcache.cached_total_length, train->gcache.cached_weight, train->gcache.cached_power,
			train->GetCurveSpeedLimit(), train->GetDisplayMaxSpeed(), to_underlying(_settings_game.vehicle.train_acceleration_model), train->max_age.base() - train->age.base());
		// Real platform geometry already fits every linked body. This prevents
		// inventing an overlength saving as justification for discarding capacity.
		for (uint8_t index = 0; index < 2; ++index) {
			const auto *order = train->GetOrder(index); REQUIRE(order != nullptr); REQUIRE(order->IsType(OT_GOTO_STATION));
			const auto *station = Station::Get(StationID{order->GetDestination().base()});
			const auto length = station->GetPlatformLength(station->train_station.tile);
			REQUIRE(train->gcache.cached_total_length <= length * TILE_SIZE);
			fmt::print("SURFACE_RESUPPLY_THROUGHPUT_PLATFORM vehicle={} order={} station={} tiles={} train_length={} overhang=0 penalty_enabled={} load={} unload={}\n",
				train->index.base(), index, station->index.base(), length, train->gcache.cached_total_length, _settings_game.order.station_length_loading_penalty,
				to_underlying(order->GetLoadType()), to_underlying(order->GetUnloadType()));
		}
		const auto original_load = train->GetOrder(0)->GetLoadType();
		// Both native full-load variants wait for capacity before departure;
		// preserve their actual distinction while pricing ordinary available load.
		REQUIRE((original_load == OrderLoadType::FullLoad || original_load == OrderLoadType::FullLoadAny));
		REQUIRE(train->GetOrder(1)->GetLoadType() == OrderLoadType::NoLoad);
		const auto quote = QuoteSurfaceResupply<Commands::ModifyOrder>(TileIndex{0}, train->index, VehicleOrderID{0}, MOF_LOAD, static_cast<uint16_t>(OrderLoadType::LoadIfPossible));
		REQUIRE(quote.Succeeded()); REQUIRE(quote.GetCost() == 0);
		fmt::print("SURFACE_RESUPPLY_THROUGHPUT_ORDER vehicle={} order=0 current_load={} proposed_load={} command_cost=0 admitted=true receivers_unchanged=true capacity_unchanged=true\n",
			train->index.base(), to_underlying(original_load), to_underlying(OrderLoadType::LoadIfPossible));
		for (CargoType cargo = CargoType{0}; cargo < NUM_CARGO; ++cargo) {
			if (capacity[cargo] == 0) continue;
			fmt::print("SURFACE_RESUPPLY_THROUGHPUT_CARGO vehicle={} label={} capacity={} held={}\n", train->index.base(), CargoSpec::Get(cargo)->label.AsString(), capacity[cargo], held[cargo]);
		}
	}
	REQUIRE(heads == 3); REQUIRE(total_parts == 35);
	QuoteSurfaceCapitalRecovery(3000);
	fmt::print("SURFACE_RESUPPLY_THROUGHPUT_PROPOSAL heads=3 wagons=32 parts=35 purchases=0 sales=0 coupling=0 settings_changes=0 changed_orders=0 quoted_order_changes=3 command_cost=0 guaranteed_income=false reserve_restored=false module_capital_complete=false\n");
}
#endif /* TEST_SURFACE_ENTRY_FREIGHT_EFFICIENCY_H */
