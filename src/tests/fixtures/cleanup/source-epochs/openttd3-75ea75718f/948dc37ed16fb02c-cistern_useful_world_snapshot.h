/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file cistern_useful_world_snapshot.h Read-only original Cistern route observations for native file boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DDB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Native pools retain every quantity; the complete saved chunks supplement these named diagnostic values.
 */
#ifndef TESTS_CISTERN_USEFUL_WORLD_SNAPSHOT_H
#define TESTS_CISTERN_USEFUL_WORLD_SNAPSHOT_H
#include "cistern_useful_route_fixture.h"
#include "../engine_base.h"
#include "../town.h"
#include "../economy_base.h"

/** @param label Original four-byte cargo/recipe meaning. @return Lossless big-endian integer representation, independent of display text. */
inline uint32_t CisternSnapshotLabel(const BaseLabel &label)
{
	return (uint32_t{label[0]} << 24) | (uint32_t{label[1]} << 16) | (uint32_t{label[2]} << 8) | label[3];
}

/** @param world Borrowed original ordinary-loaded participants. @return Stable named native integer values; signed amounts retain their exact bits, without simulation or reconstruction. */
inline std::string CaptureCisternUsefulWorld(const CisternUsefulWorld &world)
{
	std::string result;
	/** @param name Stable unique owner/field name. @param value Exact native integer bits; unsigned conversion preserves signed values losslessly. */
	const auto add = [&](std::string_view name, uint64_t value) { result += fmt::format("{}={},", name, value); };
	const auto propellant = GetCargoTypeByLabel(CargoLabel{"PROP"}); REQUIRE(propellant != INVALID_CARGO);
	const std::array cargos{world.source.cargos[0], world.source.cargos[1], world.source.cargos[2], propellant};
	// Rebuilt cargo/engine definitions control the next native handling and
	// payment. Observe actual meanings and tariffs, never a local substitute.
	for (const auto type : cargos) {
		const auto &spec = *CargoSpec::Get(type); const auto prefix = fmt::format("cargo{}_", type);
		add(prefix + "label", CisternSnapshotLabel(spec.label)); add(prefix + "weight", spec.weight);
		add(prefix + "multiplier", spec.multiplier); add(prefix + "payment", spec.current_payment.base());
		add(prefix + "classes", spec.classes.base()); add(prefix + "freight", spec.is_freight);
	}
	add("running_train_price", _price[Price::RunningTrainDiesel].base());
	add("tick", TimerGameTick::counter); add("calendar", TimerGameCalendar::date.base()); add("calendar_fraction", TimerGameCalendar::date_fract);
	add("calendar_year", TimerGameCalendar::year.base()); add("calendar_month", TimerGameCalendar::month);
	add("economy", TimerGameEconomy::date.base()); add("economy_fraction", TimerGameEconomy::date_fract);
	add("economy_year", TimerGameEconomy::year.base()); add("economy_month", TimerGameEconomy::month); add("economy_days", TimerGameEconomy::days_since_last_month);
	add("random0", _random.state[0]); add("random1", _random.state[1]);
	// Interactive RNG is client-local and is not serialized by native DATE.
	// The pure-save observer checks it separately; it cannot define a file's
	// canonical simulation state or require a reader to restore client entropy.
	add("vehicles", Vehicle::GetNumItems()); add("industries", Industry::GetNumItems()); add("stations", Station::GetNumItems()); add("packets", CargoPacket::GetNumItems());
	add("multiple_industry_per_town", _settings_game.economy.multiple_industry_per_town);
	const std::array parts{world.source.parts[0], world.source.parts[1], world.source.parts[2], world.source.parts[3],
		world.parts[0], world.parts[1], world.parts[2], world.parts[3], world.parts[4]};
	// Both retired raw haulers and all three live modal fronts/trailers belong
	// to each boundary. A matching capsule cannot hide a changed source keeper.
	for (size_t index = 0; index < parts.size(); ++index) {
		const auto &part = *parts[index]; const auto prefix = fmt::format("part{}_", index);
		add(prefix + "id", part.index.base()); add(prefix + "first", part.First()->index.base()); add(prefix + "engine", part.engine_type.base());
		add(prefix + "owner", part.owner.base()); add(prefix + "cargo", part.cargo_type); add(prefix + "units", part.cargo.StoredCount()); add(prefix + "capacity", part.cargo_cap);
		add(prefix + "tile", part.tile.base()); add(prefix + "layer", to_underlying(GetTileLayer(part.tile)));
		add(prefix + "x", part.x_pos); add(prefix + "y", part.y_pos); add(prefix + "z", part.z_pos);
		add(prefix + "direction", to_underlying(part.direction)); add(prefix + "track", part.track.base()); add(prefix + "speed", part.cur_speed);
		add(prefix + "subspeed", part.subspeed); add(prefix + "progress", part.progress); add(prefix + "status", part.vehstatus.base());
		add(prefix + "flags", part.flags.base()); add(prefix + "order", to_underlying(part.current_order.GetType()));
		add(prefix + "destination", part.current_order.GetDestination().base()); add(prefix + "order_index", part.cur_real_order_index);
		add(prefix + "dest_tile", part.dest_tile.base()); add(prefix + "profit", part.profit_this_year.base());
	}
	// Observe each actual engine once, including native launch/availability,
	// refit and load definitions that ordinary loading must faithfully rebuild.
	std::set<EngineID> engines;
	for (const auto *part : parts) engines.insert(part->engine_type);
	for (const auto id : engines) {
		const auto &engine = *Engine::Get(id); const auto prefix = fmt::format("engine{}_", id.base());
		add(prefix + "intro", engine.intro_date.base()); add(prefix + "base_intro", engine.info.base_intro.base());
		add(prefix + "flags", engine.flags.base()); add(prefix + "available", engine.company_avail.base()); add(prefix + "age", engine.age);
		add(prefix + "reliability", engine.reliability); add(prefix + "refits", engine.info.refit_mask.base()); add(prefix + "cargo", engine.info.cargo_type);
		add(prefix + "load", engine.info.load_amount); add(prefix + "misc", engine.info.misc_flags.base());
	}
	const auto &company = *Company::Get(world.parts[2]->owner);
	add("cash", company.money.base()); add("loan", company.current_loan.base()); add("quarters", company.num_valid_stat_ent);
	// Native quarter rollover retains its own entries. Include current and
	// every valid historical account instead of manufacturing a cumulative sum.
	for (uint index = 0; index <= company.num_valid_stat_ent; ++index) {
		const auto &account = index == 0 ? company.cur_economy : company.old_economy[index - 1];
		const auto prefix = index == 0 ? std::string{"current_"} : fmt::format("quarter{}_", index - 1);
		add(prefix + "income", account.income.base()); add(prefix + "expenses", account.expenses.base());
		for (const auto cargo : cargos) add(prefix + fmt::format("cargo{}", cargo), account.delivered_cargo[cargo]);
	}
	add("total_income", CisternSourceAccounts(company, cargos[2]).first.base()); add("total_delivered", CisternSourceAccounts(company, cargos[2]).second);
	const std::array industries{world.source.industries[0], world.source.industries[1], world.source.industries[2], world.receiver};
	// Every real industry slot and its valid native history accompanies both
	// recipe owners. Reading accepted history must never allocate missing data.
	for (size_t index = 0; index < industries.size(); ++index) {
		const auto &industry = *industries[index]; const auto prefix = fmt::format("industry{}_", index);
		add(prefix + "id", industry.index.base()); add(prefix + "type", industry.type); add(prefix + "tile", industry.location.tile.base());
		add(prefix + "width", industry.location.w); add(prefix + "height", industry.location.h); add(prefix + "history", industry.valid_history);
		add(prefix + "counter", industry.counter); add(prefix + "level", industry.prod_level); add(prefix + "accepted", industry.accepted.size()); add(prefix + "produced", industry.produced.size());
		for (size_t slot = 0; slot < industry.accepted.size(); ++slot) {
			const auto &entry = industry.accepted[slot]; const auto name = prefix + fmt::format("input{}_", slot);
			add(name + "cargo", entry.cargo); add(name + "waiting", entry.waiting); add(name + "accumulated", entry.accumulated_waiting);
			add(name + "last", entry.last_accepted.base()); add(name + "history", entry.history != nullptr);
			if (entry.history == nullptr) continue;
			// Only current/valid historical observations describe real business
			// periods. Complete native chunks also retain unused record bytes.
			for (uint period = 0; period < HISTORY_RECORDS; ++period) {
				if (period != THIS_MONTH && (industry.valid_history & (uint64_t{1} << period)) == 0) continue;
				const auto &history = (*entry.history)[period];
				add(name + fmt::format("h{}", period), (uint64_t{history.accepted} << 16) | history.waiting);
			}
		}
		for (size_t slot = 0; slot < industry.produced.size(); ++slot) {
			const auto &entry = industry.produced[slot]; const auto name = prefix + fmt::format("output{}_", slot);
			add(name + "cargo", entry.cargo); add(name + "waiting", entry.waiting); add(name + "rate", entry.rate);
			// Pack the two native sixteen-bit counters losslessly per named
			// period; no rounded statistic replaces original production/custody.
			for (uint period = 0; period < HISTORY_RECORDS; ++period) {
				if (period != THIS_MONTH && (industry.valid_history & (uint64_t{1} << period)) == 0) continue;
				const auto &history = entry.history[period];
				add(name + fmt::format("h{}", period), (uint64_t{history.production} << 16) | history.transported);
			}
		}
		const auto *processor = GetLayerProcessorState(industry.index); add(prefix + "processor", processor != nullptr);
		if (processor == nullptr) continue;
		add(prefix + "recipe", CisternSnapshotLabel(processor->recipe)); add(prefix + "work", processor->work_ticks); add(prefix + "fraction", processor->scale_fraction);
		add(prefix + "revision", processor->revision); add(prefix + "dispatched", processor->dispatched); add(prefix + "controller", processor->controller.base());
		for (const auto &[recipe, count] : processor->batches) add(prefix + fmt::format("batch{}", CisternSnapshotLabel(recipe)), count);
		for (const auto &[cargo, count] : processor->consumed) add(prefix + fmt::format("consumed{}", CisternSnapshotLabel(cargo)), count);
		for (const auto &[cargo, count] : processor->produced) add(prefix + fmt::format("made{}", CisternSnapshotLabel(cargo)), count);
	}
	const std::array stations{world.source.stations[0], world.source.stations[1], world.source.stations[2], world.stations[0], world.stations[1], world.stations[2]};
	// All six queues retain the actual native service/eligibility metadata for
	// every original cargo, including upstream material and undispatched PROP.
	for (size_t index = 0; index < stations.size(); ++index) {
		const auto &station = *stations[index]; const auto prefix = fmt::format("station{}_", index);
		add(prefix + "id", station.index.base()); add(prefix + "tile", station.xy.base()); add(prefix + "owner", station.owner.base()); add(prefix + "town", station.town->index.base());
		for (const auto cargo : cargos) {
			const auto &entry = station.goods[cargo]; const auto name = prefix + fmt::format("cargo{}_", cargo);
			add(name + "units", entry.TotalCount()); add(name + "fraction", entry.amount_fract); add(name + "rating", entry.rating);
			add(name + "speed", entry.last_speed); add(name + "status", entry.status.base());
		}
	}
	const auto &town = *world.source.stations[0]->town;
	add("town", town.index.base()); add("town_xy", town.xy.base()); add("town_houses", town.cache.num_houses); add("town_population", town.cache.population);
	add("deposit", world.source.deposit.base()); add("reserve", GetGeologyWorldState().deposits.Get(world.source.deposit)->remaining_reserve);
	const auto *mine = GetLayerMineState(world.source.industries[2]->index); REQUIRE(mine != nullptr);
	add("mine_work", mine->work_ticks); add("mine_fraction", mine->work_fraction); add("mine_scale", mine->scale_fraction); add("mine_revision", mine->revision);
	add("mine_extracted", mine->extracted); add("mine_dispatched", mine->dispatched); add("mine_phase", to_underlying(mine->phase));
	add("processor_digest", ComputeLayerProcessorDigest()); add("mine_digest", ComputeLayerMineDigest());
	add("utility", ComputeUtilityPowerDigest()); add("binding", ComputeTubePowerVehicleDigest()); add("invoice", ComputeTubePowerDigest()); add("source", world.power.base());
	const auto *binding = GetTubePowerVehicle(world.parts[2]->index); REQUIRE(binding != nullptr);
	add("consumer", binding->consumer.base()); add("carry", binding->remainder_mj); add("prepared_tick", binding->prepared_tick); add("consumed_tick", binding->consumed_tick);
	add("phase", to_underlying(binding->phase)); add("wait", to_underlying(binding->wait)); add("binding_source", binding->source.base());
	const auto *producer = GetUtilityPowerProducer(world.power); REQUIRE(producer != nullptr);
	add("consumed", producer->consumed_lifetime_j); add("generated", producer->generated_lifetime_j); add("reserved", producer->reserved_j);
	// Native packet-ID order preserves genuine split identities and aging.
	// History counters are never counted as a second physical cargo store.
	for (const auto *packet : CargoPacket::Iterate()) {
		const auto prefix = fmt::format("packet{}_", packet->index.base());
		add(prefix + "units", packet->Count()); add(prefix + "source", packet->GetSource().id); add(prefix + "source_type", to_underlying(packet->GetSource().type));
		add(prefix + "station", packet->GetFirstStation().base()); add(prefix + "age", packet->GetPeriodsInTransit()); add(prefix + "purpose", packet->GetCompanySupplyOrder());
	}
	return result;
}
#endif /* TESTS_CISTERN_USEFUL_WORLD_SNAPSHOT_H */
