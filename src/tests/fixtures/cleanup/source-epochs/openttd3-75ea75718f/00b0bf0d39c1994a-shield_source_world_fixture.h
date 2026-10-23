/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file shield_source_world_fixture.h Borrowed original1985 RARE source identities and lossless native file observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15EBB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Borrowed observations are read-only. Explicit floor/complete-opportunity exports retain their original native owners.
 */
#ifndef TESTS_SHIELD_SOURCE_WORLD_FIXTURE_H
#define TESTS_SHIELD_SOURCE_WORLD_FIXTURE_H
#include <charconv>
#include "../multilayer_industry.h"
#include "../layer_industry_processor.h"
#include "../excavation_state.h"
#include "../industry.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../town.h"
#include "../engine_base.h"
#include "../underground_vehicle.h"
#include "../transport_tube.h"
#include "../economy_base.h"
#include "../train.h"
#include "../cargopacket.h"
#include "../core/random_func.hpp"
#include "../timer/timer_game_calendar.h"
#include "../timer/timer_game_economy.h"
#include "../timer/timer_game_tick.h"
#include "movement_fixture.h"

/** Borrowed actual original source owners; later modal construction retains these exact native identities. */
struct ShieldSourceWorld {
	std::array<Train *, 2> parts{}; ///< Original actual Deep hauler4 and coupled sealed wagon6.
	std::array<Industry *, 2> industries{}; ///< Original finite Deep RARE mine and actual Surface ELEC receiver.
	GeologyDepositID deposit{UINT32_MAX}; ///< Original geological reserve identity, never an industry-local replacement.
	TileIndex home = INVALID_TILE; ///< Actual original paid Deep depot, not an authored future arrival.
	CargoType cargo = INVALID_CARGO; ///< Exact ordinary-loaded RARE registry meaning.
};

/** @param tiles Explicit scenario-owned prepared floor; retain every original reserve/claim. Ordinary source-file readers never call this preparation boundary. */
void OpenShieldSourceFloor(std::span<const TileIndex> tiles);
/** Advance one complete native source/route opportunity: real clocks/all vehicles, landscaping limits and exactly one ordinary industry dispatcher. */
void RunShieldSourceOpportunity();

/** @param name Required original receipt field list. @param count Exact complete field count. @return Checked unsigned native observations; partial/truncated identities fail before any work. */
inline std::vector<uint64_t> ShieldSourceValues(const char *name, size_t count)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr); const char *end = text + strlen(text); std::vector<uint64_t> values;
	// The complete original suffix must parse. A truncated list cannot choose
	// another participant or silently discard an unverified source identity.
	while (text < end) {
		uint64_t value = 0; const auto parsed = std::from_chars(text, end, value); REQUIRE(parsed.ec == std::errc{}); values.push_back(value);
		if (parsed.ptr == end) { text = end; break; }
		REQUIRE(*parsed.ptr == ','); text = parsed.ptr + 1; REQUIRE(text < end);
	}
	REQUIRE(values.size() == count); return values;
}

/** @param source_only Require original stopped/empty source-only pools; later route consumers must instead enforce their explicit expanded pools. @return Borrowed exact ordinary-loaded native participants, without construction or metadata repair. */
inline ShieldSourceWorld LoadShieldSourceWorld(bool source_only = true)
{
	ShieldSourceWorld world; const auto parts = ShieldSourceValues("OTTD_SHIELD_SOURCE_PARTS", 2), references = ShieldSourceValues("OTTD_SHIELD_SOURCE_REFERENCES", 4);
	// Source acceptance forbids extra participants. Later actual movement may
	// reuse the original carrier; only its explicitly expanded route gate may
	// replace this source-only stopped/empty/full-count boundary.
	if (source_only) { REQUIRE(Vehicle::GetNumItems() == 2); REQUIRE(Industry::GetNumItems() == 2); REQUIRE(Station::GetNumItems() == 0); REQUIRE(CargoPacket::GetNumItems() == 0); }
	for (size_t index = 0; index < parts.size(); ++index) {
		REQUIRE(parts[index] < UINT32_MAX); world.parts[index] = Train::GetIfValid(VehicleID{static_cast<uint32_t>(parts[index])}); REQUIRE(world.parts[index] != nullptr);
		REQUIRE(references[index] < UINT16_MAX); world.industries[index] = Industry::GetIfValid(IndustryID{static_cast<uint16_t>(references[index])}); REQUIRE(world.industries[index] != nullptr);
	}
	REQUIRE(references[2] < UINT32_MAX); REQUIRE(references[3] < Map::Size());
	world.deposit = GeologyDepositID{static_cast<uint32_t>(references[2])}; world.home = TileIndex{static_cast<uint32_t>(references[3])};
	world.cargo = GetCargoTypeByLabel(CargoLabel{"RARE"}); REQUIRE(world.cargo != INVALID_CARGO);
	REQUIRE(world.parts[0]->engine_type == GetUndergroundEngine(4)); REQUIRE(world.parts[1]->engine_type == GetUndergroundEngine(6));
	REQUIRE(world.parts[1]->First() == world.parts[0]); REQUIRE(world.parts[1]->Next() == nullptr); REQUIRE(world.parts[1]->cargo_type == world.cargo);
	REQUIRE(IsTrainOnItsTiles(world.parts[0])); REQUIRE_FALSE(world.parts[0]->vehstatus.Test(VehState::Crashed));
	if (source_only) {
		REQUIRE(world.parts[0]->IsChainInDepot()); REQUIRE(world.parts[0]->tile == world.home); REQUIRE(world.parts[0]->vehstatus.Test(VehState::Stopped));
		for (const auto *part : world.parts) REQUIRE(part->cargo.StoredCount() == 0);
	}
	REQUIRE(world.industries[0]->type == GetLayerMineIndustryType(CargoLabel{"RARE"})); REQUIRE(world.industries[0]->produced[0].cargo == world.cargo);
	const auto *mine = GetLayerMineState(world.industries[0]->index); REQUIRE(mine != nullptr); REQUIRE(mine->deposit == world.deposit);
	const auto *deposit = GetGeologyWorldState().deposits.Get(world.deposit); REQUIRE(deposit != nullptr); REQUIRE(deposit->resource == CargoLabel{"RARE"});
	REQUIRE(deposit->layer == MapLayer::DeepUnderground); REQUIRE(GetTileLayer(world.home) == MapLayer::DeepUnderground);
	const auto *recipe = GetLayerProcessorRecipe(*world.industries[1]); REQUIRE(recipe != nullptr); REQUIRE(recipe->recipe.id == MultiLayerRecipeID{"ELEC"});
	return world;
}

/** @param label Original native four-byte cargo/recipe meaning. @return Exact big-endian integer bits, independent of display text. */
inline uint32_t ShieldSourceLabel(const BaseLabel &label)
{
	return (uint32_t{label[0]} << 24) | (uint32_t{label[1]} << 16) | (uint32_t{label[2]} << 8) | label[3];
}

/** @param world Borrowed original source owners. @return Canonical named native observations; unsigned transport preserves signed values losslessly without advancing or repairing any owner. */
inline std::string CaptureShieldSourceWorld(const ShieldSourceWorld &world)
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
	for (const auto label : {CargoLabel{"RARE"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}}) {
		const auto cargo = GetCargoTypeByLabel(label); REQUIRE(cargo != INVALID_CARGO); const auto &spec = *CargoSpec::Get(cargo); const auto prefix = fmt::format("cargo{}_", cargo);
		add(prefix + "label", ShieldSourceLabel(spec.label)); add(prefix + "weight", spec.weight); add(prefix + "multiplier", spec.multiplier);
		add(prefix + "payment", spec.current_payment.base()); add(prefix + "classes", spec.classes.base()); add(prefix + "freight", spec.is_freight);
	}
	for (const auto id : {world.parts[0]->engine_type, world.parts[1]->engine_type, GetTubeEngine(12)}) {
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
		}
		for (size_t slot = 0; slot < industry.produced.size(); ++slot) {
			const auto &entry = industry.produced[slot]; const auto name = prefix + fmt::format("output{}_", slot);
			add(name + "cargo", entry.cargo); add(name + "waiting", entry.waiting); add(name + "rate", entry.rate);
			add(name + "produced", entry.history[THIS_MONTH].production); add(name + "transported", entry.history[THIS_MONTH].transported);
		}
		const auto *processor = GetLayerProcessorState(industry.index); add(prefix + "processor", processor != nullptr); if (processor == nullptr) continue;
		add(prefix + "recipe", ShieldSourceLabel(processor->recipe)); add(prefix + "work", processor->work_ticks); add(prefix + "fraction", processor->scale_fraction); add(prefix + "revision", processor->revision); add(prefix + "dispatched", processor->dispatched);
		for (const auto &[recipe, count] : processor->batches) add(prefix + fmt::format("batch{}", ShieldSourceLabel(recipe)), count);
		for (const auto &[cargo, count] : processor->consumed) add(prefix + fmt::format("consumed{}", ShieldSourceLabel(cargo)), count);
		for (const auto &[cargo, count] : processor->produced) add(prefix + fmt::format("made{}", ShieldSourceLabel(cargo)), count);
	}
	add("home", world.home.base()); add("deposit", world.deposit.base()); add("reserve", GetGeologyWorldState().deposits.Get(world.deposit)->remaining_reserve);
	const auto &mine = *GetLayerMineState(world.industries[0]->index);
	add("mine_work", mine.work_ticks); add("mine_fraction", mine.work_fraction); add("mine_scale", mine.scale_fraction); add("mine_revision", mine.revision);
	add("mine_extracted", mine.extracted); add("mine_dispatched", mine.dispatched); add("mine_phase", to_underlying(mine.phase));
	add("mine_digest", ComputeLayerMineDigest()); add("processor_digest", ComputeLayerProcessorDigest());
	const auto &town = *world.industries[1]->town; add("town", town.index.base()); add("town_xy", town.xy.base()); add("town_houses", town.cache.num_houses); add("town_population", town.cache.population);
	return result;
}
#endif /* TESTS_SHIELD_SOURCE_WORLD_FIXTURE_H */
