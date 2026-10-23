/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file forge_useful_world_snapshot.h Read-only complete useful Forge owners for original native file boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FEB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Original raw observations remain unchanged; complete native chunks independently protect every serialized owner.
 */
#ifndef TESTS_FORGE_USEFUL_WORLD_SNAPSHOT_H
#define TESTS_FORGE_USEFUL_WORLD_SNAPSHOT_H
#include "forge_useful_route_fixture.h"
#include "../effectvehicle_base.h"

/** @param world Borrowed original raw and actual paid modal participants. @return Canonical unique named integer observations of every actual source/modal/receiver/account/power/packet owner, without advancing or repairing state. */
inline std::string CaptureForgeUsefulWorld(const ForgeUsefulWorld &world)
{
	std::string result = CaptureForgeRawDelivery(world.raw);
	/** @param name Unique native owner/field name supplementing the unchanged original raw observations. @param value Exact integer bits, including losslessly transported signed Money. */
	const auto add = [&](std::string_view name, uint64_t value) { result += fmt::format("{}={},", name, value); };
	uint64_t trains = 0, effects = 0;
	// VEHS natively saves visual-effect vehicles as well as train parts. Keep
	// every actual effect identity and each SlVehicleEffect serialized field;
	// normalization or deleting smoke would change the original moving world.
	for (const auto *vehicle : Vehicle::Iterate()) {
		if (vehicle->type == VehicleType::Train) { ++trains; continue; }
		REQUIRE(vehicle->type == VehicleType::Effect); ++effects; const auto *effect = EffectVehicle::From(vehicle); const auto name = fmt::format("effect{}_", effect->index.base());
		add(name + "id", effect->index.base()); add(name + "subtype", effect->subtype); add(name + "tile", effect->tile.base()); add(name + "x", effect->x_pos); add(name + "y", effect->y_pos); add(name + "z", effect->z_pos);
		add(name + "sprite", effect->sprite_cache.sprite_seq.seq[0].sprite); add(name + "progress", effect->progress); add(name + "status", effect->vehstatus.base());
		add(name + "animation", effect->animation_state); add(name + "subanimation", effect->animation_substate); add(name + "spritenum", effect->spritenum);
	}
	add("train_vehicles", trains); add("effect_vehicles", effects);
	// The original four raw parts and every current/old company bucket already
	// belong to the shared source snapshot. Add the complete five new parts so
	// native continuation cannot hide a lost conventional trailer or order.
	for (size_t index = 0; index < world.parts.size(); ++index) {
		const auto &part = *world.parts[index]; const auto prefix = fmt::format("part{}_", index + 4);
		add(prefix + "id", part.index.base()); add(prefix + "first", part.First()->index.base()); add(prefix + "engine", part.engine_type.base()); add(prefix + "owner", part.owner.base());
		add(prefix + "cargo", part.cargo_type); add(prefix + "units", part.cargo.StoredCount()); add(prefix + "capacity", part.cargo_cap);
		add(prefix + "tile", part.tile.base()); add(prefix + "layer", to_underlying(GetTileLayer(part.tile))); add(prefix + "x", part.x_pos); add(prefix + "y", part.y_pos); add(prefix + "z", part.z_pos);
		add(prefix + "direction", to_underlying(part.direction)); add(prefix + "track", part.track.base()); add(prefix + "speed", part.cur_speed); add(prefix + "subspeed", part.subspeed); add(prefix + "progress", part.progress);
		add(prefix + "status", part.vehstatus.base()); add(prefix + "flags", part.flags.base()); add(prefix + "order", to_underlying(part.current_order.GetType())); add(prefix + "destination", part.current_order.GetDestination().base());
		add(prefix + "order_index", part.cur_real_order_index); add(prefix + "dest_tile", part.dest_tile.base()); add(prefix + "profit", part.profit_this_year.base());
	}
	const auto &company = *Company::Get(world.raw.source.parts[0]->owner); add("total_income", ForgeRawAccounts(company, world.raw.source.cargos[2]).first.base());
	for (size_t index = 0; index < world.raw.source.cargos.size(); ++index) add(fmt::format("total_delivered{}", index), ForgeRawAccounts(company, world.raw.source.cargos[index]).second);
	const auto &industry = *world.receiver; const std::string prefix = "industry3_";
	add(prefix + "id", industry.index.base()); add(prefix + "type", industry.type); add(prefix + "tile", industry.location.tile.base()); add(prefix + "width", industry.location.w); add(prefix + "height", industry.location.h);
	add(prefix + "counter", industry.counter); add(prefix + "history", industry.valid_history); add(prefix + "inputs", industry.accepted.size()); add(prefix + "outputs", industry.produced.size());
	// Native ELEC input/output and every valid receipt/production period are
	// physical or historical owners, never another store of those same goods.
	for (size_t slot = 0; slot < industry.accepted.size(); ++slot) {
		const auto &entry = industry.accepted[slot]; const auto name = prefix + fmt::format("input{}_", slot);
		add(name + "cargo", entry.cargo); add(name + "waiting", entry.waiting); add(name + "accumulated", entry.accumulated_waiting); add(name + "last", entry.last_accepted.base()); add(name + "history", entry.history != nullptr);
		if (entry.history == nullptr) continue;
		for (uint period = 0; period < HISTORY_RECORDS; ++period) {
			// Current and valid native business periods supply receipt data;
			// uninitialized legacy slots cannot invent another received balance.
			if (period != THIS_MONTH && (industry.valid_history & (uint64_t{1} << period)) == 0) continue;
			const auto &history = (*entry.history)[period]; add(name + fmt::format("h{}", period), (uint64_t{history.accepted} << 16) | history.waiting);
		}
	}
	for (size_t slot = 0; slot < industry.produced.size(); ++slot) {
		const auto &entry = industry.produced[slot]; const auto name = prefix + fmt::format("output{}_", slot);
		add(name + "cargo", entry.cargo); add(name + "waiting", entry.waiting); add(name + "rate", entry.rate);
		for (uint period = 0; period < HISTORY_RECORDS; ++period) {
			// The same valid-period rule preserves actual manufacturing and
			// dispatch history without treating those counters as inventory.
			if (period != THIS_MONTH && (industry.valid_history & (uint64_t{1} << period)) == 0) continue;
			const auto &history = entry.history[period]; add(name + fmt::format("h{}", period), (uint64_t{history.production} << 16) | history.transported);
		}
	}
	const auto *processor = GetLayerProcessorState(industry.index); REQUIRE(processor != nullptr);
	add(prefix + "recipe", ShieldSourceLabel(processor->recipe)); add(prefix + "work", processor->work_ticks); add(prefix + "fraction", processor->scale_fraction); add(prefix + "revision", processor->revision); add(prefix + "dispatched", processor->dispatched);
	for (const auto &[recipe, count] : processor->batches) add(prefix + fmt::format("batch{}", ShieldSourceLabel(recipe)), count);
	for (const auto &[cargo, count] : processor->consumed) add(prefix + fmt::format("consumed{}", ShieldSourceLabel(cargo)), count);
	for (const auto &[cargo, count] : processor->produced) add(prefix + fmt::format("made{}", ShieldSourceLabel(cargo)), count);
	// All five actual stations retain queues and native service eligibility
	// for both raw meanings, RFMT and the absent ELEC recipe inputs/output.
	for (const auto *station : Station::Iterate()) {
		const auto name = fmt::format("station{}_", station->index.base()); add(name + "id", station->index.base()); add(name + "tile", station->xy.base()); add(name + "owner", station->owner.base()); add(name + "town", station->town->index.base());
		for (const auto label : {CargoLabel{"CORE"}, CargoLabel{"COAL"}, CargoLabel{"RFMT"}, CargoLabel{"RARE"}, CargoLabel{"ELEC"}}) {
			const auto cargo = GetCargoTypeByLabel(label); REQUIRE(cargo != INVALID_CARGO); const auto &entry = station->goods[cargo]; const auto key = name + fmt::format("cargo{}_", cargo);
			add(key + "units", entry.TotalCount()); add(key + "fraction", entry.amount_fract); add(key + "rating", entry.rating); add(key + "speed", entry.last_speed); add(key + "status", entry.status.base());
		}
	}
	add("utility", ComputeUtilityPowerDigest()); add("binding", ComputeTubePowerVehicleDigest()); add("invoice", ComputeTubePowerDigest()); add("source", world.power.base());
	const auto *binding = GetTubePowerVehicle(world.parts[2]->index); REQUIRE(binding != nullptr);
	add("consumer", binding->consumer.base()); add("carry", binding->remainder_mj); add("prepared_tick", binding->prepared_tick); add("consumed_tick", binding->consumed_tick); add("phase", to_underlying(binding->phase)); add("wait", to_underlying(binding->wait)); add("binding_source", binding->source.base());
	const auto *producer = GetUtilityPowerProducer(world.power); REQUIRE(producer != nullptr); add("consumed", producer->consumed_lifetime_j); add("generated", producer->generated_lifetime_j); add("reserved", producer->reserved_j);
	// Shared raw observations already retain exact packet IDs/counts/age/first
	// station/feeder/purpose. Native source ID/type additionally distinguish
	// original CORE/Coal packets from actual CFMT RFMT during modal transfer.
	for (const auto *packet : CargoPacket::Iterate()) {
		const auto name = fmt::format("packet{}_", packet->index.base()); add(name + "source", packet->GetSource().id); add(name + "source_type", to_underlying(packet->GetSource().type));
	}
	return result;
}
#endif /* TESTS_FORGE_USEFUL_WORLD_SNAPSHOT_H */
