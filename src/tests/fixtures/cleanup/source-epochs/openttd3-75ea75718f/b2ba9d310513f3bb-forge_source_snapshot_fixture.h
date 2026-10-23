/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file forge_source_snapshot_fixture.h Read-only complete original Forge raw-source observations for native file boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Native pools own all quantities; complete chunks independently retain every serialized owner.
 */
#ifndef TESTS_FORGE_SOURCE_SNAPSHOT_FIXTURE_H
#define TESTS_FORGE_SOURCE_SNAPSHOT_FIXTURE_H
#include "forge_source_world_fixture.h"

/** @param world Borrowed original source owners. @return Canonical named native observations; unsigned transport preserves signed values losslessly without advancing or repairing any owner. */
inline std::string CaptureForgeSourceWorld(const ForgeSourceWorld &world)
{
	std::string result;
	/** @param name Stable unique owner/field name. @param value Exact native integer bits, including lossless signed Money conversion. */
	const auto add = [&](std::string_view name, uint64_t value) { result += fmt::format("{}={},", name, value); };
	add("tick", TimerGameTick::counter); add("calendar", TimerGameCalendar::date.base()); add("calendar_fraction", TimerGameCalendar::date_fract);
	add("calendar_year", TimerGameCalendar::year.base()); add("calendar_month", TimerGameCalendar::month);
	add("economy", TimerGameEconomy::date.base()); add("economy_fraction", TimerGameEconomy::date_fract); add("economy_year", TimerGameEconomy::year.base());
	add("economy_month", TimerGameEconomy::month); add("economy_days", TimerGameEconomy::days_since_last_month);
	add("random0", _random.state[0]); add("random1", _random.state[1]);
	add("vehicles", Vehicle::GetNumItems()); add("industries", Industry::GetNumItems()); add("stations", Station::GetNumItems()); add("packets", CargoPacket::GetNumItems());
	// Rebuilt actual cargo/engine metadata affects the next real operation.
	// File positions alone cannot prove correct native content restoration.
	for (const auto label : {CargoLabel{"CORE"}, CargoLabel{"COAL"}, CargoLabel{"RFMT"}, CargoLabel{"BAUX"}, CargoLabel{"NORE"}}) {
		const auto cargo = GetCargoTypeByLabel(label); REQUIRE(cargo != INVALID_CARGO); const auto &spec = *CargoSpec::Get(cargo); const auto prefix = fmt::format("cargo{}_", cargo);
		add(prefix + "label", ShieldSourceLabel(spec.label)); add(prefix + "weight", spec.weight); add(prefix + "multiplier", spec.multiplier);
		add(prefix + "payment", spec.current_payment.base()); add(prefix + "classes", spec.classes.base()); add(prefix + "freight", spec.is_freight);
	}
	for (const auto id : {world.parts[0]->engine_type, world.parts[1]->engine_type, GetTubeEngine(13)}) {
		const auto &engine = *Engine::Get(id); const auto prefix = fmt::format("engine{}_", id.base());
		add(prefix + "intro", engine.intro_date.base()); add(prefix + "base_intro", engine.info.base_intro.base()); add(prefix + "available", engine.company_avail.base());
		add(prefix + "flags", engine.flags.base()); add(prefix + "age", engine.age); add(prefix + "reliability", engine.reliability); add(prefix + "refits", engine.info.refit_mask.base());
		add(prefix + "cargo", engine.info.cargo_type); add(prefix + "load", engine.info.load_amount);
	}
	for (size_t index = 0; index < world.parts.size(); ++index) {
		const auto &part = *world.parts[index]; const auto prefix = fmt::format("part{}_", index);
		add(prefix + "id", part.index.base()); add(prefix + "first", part.First()->index.base()); add(prefix + "engine", part.engine_type.base()); add(prefix + "owner", part.owner.base());
		add(prefix + "cargo", part.cargo_type); add(prefix + "units", part.cargo.StoredCount()); add(prefix + "capacity", part.cargo_cap);
		add(prefix + "tile", part.tile.base()); add(prefix + "layer", to_underlying(GetTileLayer(part.tile))); add(prefix + "x", part.x_pos); add(prefix + "y", part.y_pos); add(prefix + "z", part.z_pos);
		add(prefix + "direction", to_underlying(part.direction)); add(prefix + "track", part.track.base()); add(prefix + "speed", part.cur_speed); add(prefix + "subspeed", part.subspeed);
		add(prefix + "progress", part.progress); add(prefix + "status", part.vehstatus.base()); add(prefix + "flags", part.flags.base()); add(prefix + "order", to_underlying(part.current_order.GetType()));
		add(prefix + "destination", part.current_order.GetDestination().base()); add(prefix + "order_index", part.cur_real_order_index); add(prefix + "dest_tile", part.dest_tile.base()); add(prefix + "profit", part.profit_this_year.base());
	}
	const auto &company = *Company::Get(world.parts[0]->owner); add("cash", company.money.base()); add("loan", company.current_loan.base()); add("quarters", company.num_valid_stat_ent);
	add("running_train_price", _price[Price::RunningTrainDiesel].base());
	add("industry_scale", _settings_game.economy.industry_cargo_scale); add("economy_fluct", _economy.fluct);
	// Keep every actual native quarter and delivered-cargo bucket, including
	// zeros; a loader cannot hide an invented receipt in another cargo meaning.
	for (uint index = 0; index <= company.num_valid_stat_ent; ++index) {
		const auto &account = index == 0 ? company.cur_economy : company.old_economy[index - 1]; const auto prefix = index == 0 ? std::string{"current_"} : fmt::format("quarter{}_", index - 1);
		add(prefix + "income", account.income.base()); add(prefix + "expenses", account.expenses.base());
		for (const CargoType cargo : EnumRange(NUM_CARGO)) add(prefix + fmt::format("cargo{}", cargo), account.delivered_cargo[cargo]);
	}
	// Physical industry slots retain all actual cargo. Recipe history is a
	// separate historical owner, never another store of those same raw units.
	for (size_t index = 0; index < world.industries.size(); ++index) {
		const auto &industry = *world.industries[index]; const auto prefix = fmt::format("industry{}_", index);
		add(prefix + "id", industry.index.base()); add(prefix + "type", industry.type); add(prefix + "tile", industry.location.tile.base()); add(prefix + "width", industry.location.w); add(prefix + "height", industry.location.h);
		add(prefix + "counter", industry.counter); add(prefix + "history", industry.valid_history); add(prefix + "inputs", industry.accepted.size()); add(prefix + "outputs", industry.produced.size());
		for (size_t slot = 0; slot < industry.accepted.size(); ++slot) {
			const auto &entry = industry.accepted[slot]; const auto name = prefix + fmt::format("input{}_", slot);
			add(name + "cargo", entry.cargo); add(name + "waiting", entry.waiting); add(name + "accumulated", entry.accumulated_waiting); add(name + "last", entry.last_accepted.base());
			add(name + "history", entry.history != nullptr); if (entry.history == nullptr) continue;
			// Current and valid historical periods retain real native receipts;
			// unused legacy periods cannot invent another input balance.
			for (uint period = 0; period < HISTORY_RECORDS; ++period) {
				if (period != THIS_MONTH && (industry.valid_history & (uint64_t{1} << period)) == 0) continue;
				const auto &history = (*entry.history)[period]; add(name + fmt::format("h{}", period), (uint64_t{history.accepted} << 16) | history.waiting);
			}
		}
		for (size_t slot = 0; slot < industry.produced.size(); ++slot) {
			const auto &entry = industry.produced[slot]; const auto name = prefix + fmt::format("output{}_", slot);
			add(name + "cargo", entry.cargo); add(name + "waiting", entry.waiting); add(name + "rate", entry.rate);
			add(name + "produced", entry.history[THIS_MONTH].production); add(name + "transported", entry.history[THIS_MONTH].transported);
			// Production and dispatch history are observations, not a second
			// store of the actual reserve-backed raw units in this output slot.
			for (uint period = 0; period < HISTORY_RECORDS; ++period) {
				if (period != THIS_MONTH && (industry.valid_history & (uint64_t{1} << period)) == 0) continue;
				const auto &history = entry.history[period]; add(name + fmt::format("h{}", period), (uint64_t{history.production} << 16) | history.transported);
			}
		}
		const auto *processor = GetLayerProcessorState(industry.index); add(prefix + "processor", processor != nullptr); if (processor == nullptr) continue;
		add(prefix + "recipe", ShieldSourceLabel(processor->recipe)); add(prefix + "work", processor->work_ticks); add(prefix + "fraction", processor->scale_fraction); add(prefix + "revision", processor->revision); add(prefix + "dispatched", processor->dispatched);
		for (const auto &[recipe, count] : processor->batches) add(prefix + fmt::format("batch{}", ShieldSourceLabel(recipe)), count);
		for (const auto &[cargo, count] : processor->consumed) add(prefix + fmt::format("consumed{}", ShieldSourceLabel(cargo)), count);
		for (const auto &[cargo, count] : processor->produced) add(prefix + fmt::format("made{}", ShieldSourceLabel(cargo)), count);
	}
	// Both independently claimed native sources retain exact immutable and
	// remaining reserves, source work/fractions and original home identities.
	// Canonical geology additionally covers all prepared terrain/other claims.
	for (size_t index = 0; index < world.deposits.size(); ++index) {
		const auto suffix = std::to_string(index), prefix = fmt::format("mine{}_", index);
		const auto *deposit = GetGeologyWorldState().deposits.Get(world.deposits[index]); REQUIRE(deposit != nullptr);
		add("home" + suffix, world.homes[index].base()); add("deposit" + suffix, world.deposits[index].base());
		add("reserve" + suffix, deposit->remaining_reserve); add("initial_reserve" + suffix, deposit->initial_reserve);
		const auto *mine = GetLayerMineState(world.industries[index]->index); REQUIRE(mine != nullptr);
		add(prefix + "industry", mine->industry.base()); add(prefix + "deposit", mine->deposit.base()); add(prefix + "owner", mine->company.base());
		add(prefix + "work", mine->work_ticks); add(prefix + "fraction", mine->work_fraction); add(prefix + "scale", mine->scale_fraction); add(prefix + "revision", mine->revision);
		add(prefix + "extracted", mine->extracted); add(prefix + "dispatched", mine->dispatched); add(prefix + "phase", to_underlying(mine->phase));
	}
	add("geology", ComputeGeologyStateDigest(GetGeologyWorldState()));
	add("mine_digest", ComputeLayerMineDigest()); add("processor_digest", ComputeLayerProcessorDigest());
	const auto &town = *world.industries[2]->town; add("town", town.index.base()); add("town_xy", town.xy.base()); add("town_houses", town.cache.num_houses); add("town_population", town.cache.population);
	return result;
}
#endif /* TESTS_FORGE_SOURCE_SNAPSHOT_FIXTURE_H */
