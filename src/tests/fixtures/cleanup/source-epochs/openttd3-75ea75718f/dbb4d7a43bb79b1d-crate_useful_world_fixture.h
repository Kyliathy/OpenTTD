/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file crate_useful_world_fixture.h Borrowed original Crate participants and read-only canonical observations for ordinary useful-route files.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15CC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * This test view owns no cargo, account, source or saved schema; normal application loading must construct every participant first.
 */
#ifndef TESTS_CRATE_USEFUL_WORLD_FIXTURE_H
#define TESTS_CRATE_USEFUL_WORLD_FIXTURE_H
#include "../transport_tube.h"
#include "../industry.h"
#include "../station_base.h"
#include "../town.h"
#include "../company_base.h"
#include "../train.h"
#include "../cargopacket.h"
#include "../core/random_func.hpp"
#include "../timer/timer_game_calendar.h"
#include "../timer/timer_game_economy.h"
#include "../timer/timer_game_tick.h"

/** Borrowed original native participants; the five parts are feed/wagon/Crate/delivery/wagon, never reconstructed fixture substitutes. */
struct CrateUsefulWorld {
	std::array<Train *, 5> parts{}; ///< Original native parts in their declared semantic order.
	std::array<Industry *, 2> industries{}; ///< Original Forest and actual Sawmill, retaining native production/history.
	Town *town = nullptr; ///< Native receiver town whose real houses supply demand.
	std::array<Station *, 4> stations{}; ///< Sawmill pickup, Underground inlet/interchange and actual Surface town berth.
	UtilityProducerID power{UINT32_MAX}; ///< Original paid external provider, never a copied work/energy counter.
};

/** @param company Actual native accounting owner. @param cargo Original Goods meaning. @return Read-only income and useful delivered units across current and retained native quarters. */
inline auto CrateNativeAccounts(const Company &company, CargoType cargo)
{
	Money income = company.cur_economy.income; uint64_t delivered = company.cur_economy.delivered_cargo[cargo];
	// The real precursor plus freight route crosses a native quarter. Include
	// actual retained records instead of freezing time or treating a reset
	// current-quarter bucket as a lost shipment or a new payment.
	for (uint index = 0; index < company.num_valid_stat_ent; ++index) {
		income += company.old_economy[index].income; delivered += company.old_economy[index].delivered_cargo[cargo];
	}
	return std::pair{income, delivered};
}

/** @param world Borrowed actual writer or fully application-loaded participants. @return Named lossless canonical scalar observations; no simulation, packet, date or account is repaired or advanced. */
inline std::string CaptureCrateUsefulWorld(const CrateUsefulWorld &world)
{
	std::string snapshot;
	/** @param name Stable diagnostic field name. @param value Exact native integer bits; signed Money is converted losslessly to unsigned for transport. */
	const auto add = [&](std::string_view name, uint64_t value) { snapshot += fmt::format("{}={},", name, value); };
	const auto cargo = GetCargoTypeByLabel(CT_GOODS), wood = GetCargoTypeByLabel(CT_WOOD);
	// Native cargo definitions and derived tariffs affect the next real
	// movement/payment even though ordinary loading rebuilds them from content.
	// Observing them prevents a reduced producer definition from hiding behind
	// identical serialized positions or previously earned company balances.
	for (const auto type : {cargo, wood}) {
		const auto &spec = *CargoSpec::Get(type); const auto prefix = fmt::format("cargo{}_", type);
		add(prefix + "weight", spec.weight); add(prefix + "multiplier", spec.multiplier);
		add(prefix + "payment", static_cast<uint64_t>(spec.current_payment.base()));
		add(prefix + "acceptance", to_underlying(spec.town_acceptance_effect));
	}
	add("running_train_price", static_cast<uint64_t>(_price[Price::RunningTrainDiesel].base()));
	add("utility", ComputeUtilityPowerDigest()); add("binding", ComputeTubePowerVehicleDigest()); add("invoice", ComputeTubePowerDigest());
	add("tick", TimerGameTick::counter); add("calendar", TimerGameCalendar::date.base()); add("calendar_fraction", TimerGameCalendar::date_fract);
	add("calendar_year", TimerGameCalendar::year.base()); add("calendar_month", TimerGameCalendar::month);
	add("economy", TimerGameEconomy::date.base()); add("economy_fraction", TimerGameEconomy::date_fract);
	add("economy_year", TimerGameEconomy::year.base()); add("economy_month", TimerGameEconomy::month); add("economy_days", TimerGameEconomy::days_since_last_month);
	add("random0", _random.state[0]); add("random1", _random.state[1]);
	add("vehicles", Vehicle::GetNumItems()); add("packets", CargoPacket::GetNumItems());
	// Exact original parts include live physical, order, custody and profit
	// observations. Native split packets may gain IDs, but a saved boundary
	// must reload every existing ID and its actual current owner unchanged.
	for (size_t index = 0; index < world.parts.size(); ++index) {
		const auto &part = *world.parts[index]; const auto prefix = fmt::format("part{}_", index);
		add(prefix + "id", part.index.base()); add(prefix + "first", part.First()->index.base()); add(prefix + "engine", part.engine_type.base());
		add(prefix + "owner", part.owner.base()); add(prefix + "cargo", part.cargo_type); add(prefix + "units", part.cargo.StoredCount());
		add(prefix + "tile", part.tile.base()); add(prefix + "layer", to_underlying(GetTileLayer(part.tile)));
		add(prefix + "x", part.x_pos); add(prefix + "y", part.y_pos); add(prefix + "z", part.z_pos);
		add(prefix + "direction", to_underlying(part.direction)); add(prefix + "track", part.track.base()); add(prefix + "speed", part.cur_speed);
		add(prefix + "subspeed", part.subspeed); add(prefix + "progress", part.progress); add(prefix + "status", part.vehstatus.base());
		add(prefix + "flags", part.flags.base()); add(prefix + "order", to_underlying(part.current_order.GetType()));
		add(prefix + "destination", part.current_order.GetDestination().base()); add(prefix + "order_index", part.cur_real_order_index);
		add(prefix + "dest_tile", part.dest_tile.base()); add(prefix + "profit", static_cast<uint64_t>(part.profit_this_year.base()));
	}
	const auto &company = *Company::Get(world.parts[2]->owner);
	add("cash", static_cast<uint64_t>(company.money.base())); add("current_income", static_cast<uint64_t>(company.cur_economy.income.base()));
	add("current_expenses", static_cast<uint64_t>(company.cur_economy.expenses.base())); add("current_goods", company.cur_economy.delivered_cargo[cargo]);
	add("current_wood", company.cur_economy.delivered_cargo[wood]); add("quarters", company.num_valid_stat_ent);
	// Historical accounts remain original native owners through the quarter
	// transition; no local accumulated balance may replace any stored entry.
	for (uint index = 0; index < company.num_valid_stat_ent; ++index) {
		const auto &account = company.old_economy[index]; const auto prefix = fmt::format("quarter{}_", index);
		add(prefix + "income", static_cast<uint64_t>(account.income.base())); add(prefix + "expenses", static_cast<uint64_t>(account.expenses.base()));
		add(prefix + "goods", account.delivered_cargo[cargo]); add(prefix + "wood", account.delivered_cargo[wood]);
	}
	const auto [income, delivered] = CrateNativeAccounts(company, cargo);
	add("total_income", static_cast<uint64_t>(income.base())); add("total_delivered", delivered);
	add("town", world.town->index.base()); add("town_xy", world.town->xy.base());
	add("town_houses", world.town->cache.num_houses); add("town_population", world.town->cache.population);
	// Both original industry inventories/history and all actual modal queues
	// accompany the vehicles; a loader cannot recreate a useful source or
	// manufacture a destination merely by accepting matching capsule counts.
	for (size_t index = 0; index < world.industries.size(); ++index) {
		const auto &industry = *world.industries[index]; const auto prefix = fmt::format("industry{}_", index);
		add(prefix + "id", industry.index.base()); add(prefix + "type", industry.type); add(prefix + "tile", industry.location.tile.base());
		add(prefix + "width", industry.location.w); add(prefix + "height", industry.location.h);
		add(prefix + "cargo", industry.produced[0].cargo); add(prefix + "waiting", industry.produced[0].waiting); add(prefix + "rate", industry.produced[0].rate);
		add(prefix + "produced", industry.produced[0].history[THIS_MONTH].production); add(prefix + "transported", industry.produced[0].history[THIS_MONTH].transported);
	}
	for (size_t index = 0; index < world.stations.size(); ++index) {
		const auto &station = *world.stations[index]; const auto &entry = station.goods[cargo]; const auto prefix = fmt::format("station{}_", index);
		add(prefix + "id", station.index.base()); add(prefix + "tile", station.xy.base()); add(prefix + "owner", station.owner.base());
		add(prefix + "town", station.town->index.base()); add(prefix + "goods", entry.TotalCount()); add(prefix + "fraction", entry.amount_fract);
		add(prefix + "rating", entry.rating); add(prefix + "speed", entry.last_speed); add(prefix + "status", entry.status.base());
	}
	add("source", world.power.base()); const auto *binding = GetTubePowerVehicle(world.parts[2]->index); REQUIRE(binding != nullptr);
	add("carry", binding->remainder_mj); const auto *producer = GetUtilityPowerProducer(world.power); REQUIRE(producer != nullptr);
	add("consumed", producer->consumed_lifetime_j);
	// The canonical packet pool is visited in native ID order. Preserve every
	// live packet's identity, units, origin and age, including legitimate splits
	// that cannot be reduced to the old Mole fixture's single-packet assumption.
	for (const auto *packet : CargoPacket::Iterate()) {
		const auto prefix = fmt::format("packet{}_", packet->index.base());
		add(prefix + "units", packet->Count()); add(prefix + "source", packet->GetSource().id); add(prefix + "source_type", to_underlying(packet->GetSource().type));
		add(prefix + "station", packet->GetFirstStation().base()); add(prefix + "age", packet->GetPeriodsInTransit());
		add(prefix + "purpose", packet->GetCompanySupplyOrder());
	}
	return snapshot;
}
#endif /* TESTS_CRATE_USEFUL_WORLD_FIXTURE_H */
