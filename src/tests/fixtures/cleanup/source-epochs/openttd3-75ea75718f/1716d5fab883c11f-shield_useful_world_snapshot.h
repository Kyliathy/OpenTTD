/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file shield_useful_world_snapshot.h Read-only complete useful Shield owner observations for ordinary native files.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15EDB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Original source observations remain shared; complete native chunks independently retain every serialized owner.
 */
#ifndef TESTS_SHIELD_USEFUL_WORLD_SNAPSHOT_H
#define TESTS_SHIELD_USEFUL_WORLD_SNAPSHOT_H
#include "shield_useful_route_fixture.h"

/** @param world Borrowed original native source/modal participants. @return Canonical unique named integer observations, with signed values preserved as exact unsigned bits and no simulation or repair. */
inline std::string CaptureShieldUsefulWorld(const ShieldUsefulWorld &world)
{
	std::string result = CaptureShieldSourceWorld(world.source);
	/** @param name Unique native owner/field name supplementing the original source observations. @param value Exact whole native integer bits. */
	const auto add = [&](std::string_view name, uint64_t value) { result += fmt::format("{}={},", name, value); };
	// Original parts0/1 and all real company quarters already belong to the
	// shared source snapshot. Add every field of the three actual new parts,
	// so a matching capsule cannot hide an altered conventional delivery tail.
	for (size_t index = 0; index < world.parts.size(); ++index) {
		const auto &part = *world.parts[index]; const auto prefix = fmt::format("part{}_", index + 2);
		add(prefix + "id", part.index.base()); add(prefix + "first", part.First()->index.base()); add(prefix + "engine", part.engine_type.base()); add(prefix + "owner", part.owner.base());
		add(prefix + "cargo", part.cargo_type); add(prefix + "units", part.cargo.StoredCount()); add(prefix + "capacity", part.cargo_cap);
		add(prefix + "tile", part.tile.base()); add(prefix + "layer", to_underlying(GetTileLayer(part.tile))); add(prefix + "x", part.x_pos); add(prefix + "y", part.y_pos); add(prefix + "z", part.z_pos);
		add(prefix + "direction", to_underlying(part.direction)); add(prefix + "track", part.track.base()); add(prefix + "speed", part.cur_speed); add(prefix + "subspeed", part.subspeed);
		add(prefix + "progress", part.progress); add(prefix + "status", part.vehstatus.base()); add(prefix + "flags", part.flags.base()); add(prefix + "order", to_underlying(part.current_order.GetType()));
		add(prefix + "destination", part.current_order.GetDestination().base()); add(prefix + "order_index", part.cur_real_order_index); add(prefix + "dest_tile", part.dest_tile.base()); add(prefix + "profit", part.profit_this_year.base());
	}
	const auto &company = *Company::Get(world.source.parts[0]->owner);
	Money income = company.cur_economy.income; uint64_t delivered = company.cur_economy.delivered_cargo[world.source.cargo];
	// Keep native current/valid historical totals as an additional diagnostic,
	// never a second accounting authority or replacement for the actual buckets.
	for (uint index = 0; index < company.num_valid_stat_ent; ++index) {
		income += company.old_economy[index].income; delivered += company.old_economy[index].delivered_cargo[world.source.cargo];
	}
	add("total_income", income.base()); add("total_delivered", delivered);
	// Every real accepted/produced history period accompanies the shared
	// source's complete physical industry slots and processor/mine histories.
	// Unused serialized history bytes remain covered by complete native chunks.
	for (size_t index = 0; index < world.source.industries.size(); ++index) {
		const auto &industry = *world.source.industries[index]; const auto prefix = fmt::format("industry{}_", index);
		for (size_t slot = 0; slot < industry.accepted.size(); ++slot) {
			const auto &entry = industry.accepted[slot]; const auto name = prefix + fmt::format("input{}_", slot);
			add(name + "history", entry.history != nullptr); if (entry.history == nullptr) continue;
			for (uint period = 0; period < HISTORY_RECORDS; ++period) {
				// Only current or valid historical business periods supply data;
				// uninitialized legacy periods cannot fabricate native receipts.
				if (period != THIS_MONTH && (industry.valid_history & (uint64_t{1} << period)) == 0) continue;
				const auto &history = (*entry.history)[period]; add(name + fmt::format("h{}", period), (uint64_t{history.accepted} << 16) | history.waiting);
			}
		}
		for (size_t slot = 0; slot < industry.produced.size(); ++slot) {
			const auto &entry = industry.produced[slot]; const auto name = prefix + fmt::format("output{}_", slot);
			for (uint period = 0; period < HISTORY_RECORDS; ++period) {
				// The same actual history admission applies to production and
				// transport; zero unused records do not stand for another stock.
				if (period != THIS_MONTH && (industry.valid_history & (uint64_t{1} << period)) == 0) continue;
				const auto &history = entry.history[period]; add(name + fmt::format("h{}", period), (uint64_t{history.production} << 16) | history.transported);
			}
		}
	}
	// All four native queues retain real service/rating/custody for every
	// source/recipe cargo. Cross-layer proximity never supplies cargo transport.
	for (size_t index = 0; index < world.stations.size(); ++index) {
		const auto &station = *world.stations[index]; const auto prefix = fmt::format("station{}_", index);
		add(prefix + "id", station.index.base()); add(prefix + "tile", station.xy.base()); add(prefix + "owner", station.owner.base()); add(prefix + "town", station.town->index.base());
		for (const auto label : {CargoLabel{"RARE"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}}) {
			const auto cargo = GetCargoTypeByLabel(label); REQUIRE(cargo != INVALID_CARGO); const auto &entry = station.goods[cargo]; const auto name = prefix + fmt::format("cargo{}_", cargo);
			add(name + "units", entry.TotalCount()); add(name + "fraction", entry.amount_fract); add(name + "rating", entry.rating); add(name + "speed", entry.last_speed); add(name + "status", entry.status.base());
		}
	}
	add("utility", ComputeUtilityPowerDigest()); add("binding", ComputeTubePowerVehicleDigest()); add("invoice", ComputeTubePowerDigest()); add("source", world.power.base());
	const auto *binding = GetTubePowerVehicle(world.parts[0]->index); REQUIRE(binding != nullptr);
	add("consumer", binding->consumer.base()); add("carry", binding->remainder_mj); add("prepared_tick", binding->prepared_tick); add("consumed_tick", binding->consumed_tick);
	add("phase", to_underlying(binding->phase)); add("wait", to_underlying(binding->wait)); add("binding_source", binding->source.base());
	const auto *producer = GetUtilityPowerProducer(world.power); REQUIRE(producer != nullptr);
	add("consumed", producer->consumed_lifetime_j); add("generated", producer->generated_lifetime_j); add("reserved", producer->reserved_j);
	// Genuine native split packet identities, feeder share and aging remain
	// observable even after all vehicles return: the finite mine still produces
	// real waiting inventory. A packet-free final fixture would lose that owner.
	for (const auto *packet : CargoPacket::Iterate()) {
		const auto prefix = fmt::format("packet{}_", packet->index.base());
		add(prefix + "units", packet->Count()); add(prefix + "source", packet->GetSource().id); add(prefix + "source_type", to_underlying(packet->GetSource().type));
		add(prefix + "station", packet->GetFirstStation().base()); add(prefix + "age", packet->GetPeriodsInTransit()); add(prefix + "purpose", packet->GetCompanySupplyOrder()); add(prefix + "feeder", packet->GetFeederShare().base());
	}
	return result;
}
#endif /* TESTS_SHIELD_USEFUL_WORLD_SNAPSHOT_H */
