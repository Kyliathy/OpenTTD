/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file plant_rebuild_delivery_fixture.h Actual reconstructed-plant freight, labour and complete native boundary observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group1CCCC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Helpers borrow the same actual participants; no arrival, cargo, work, clock or resource amount is assigned.
 */
#ifndef TESTS_PLANT_REBUILD_DELIVERY_FIXTURE_H
#define TESTS_PLANT_REBUILD_DELIVERY_FIXTURE_H
#include "plant_rebuild_material_fixture.h"

/** @param world Original actual carrier and reconstruction participants. @param layer Required physical cargo plane, or Invalid to count every real hold. @return Actual stored construction units on that native plane; no cross-layer catchment inference. */
inline uint32_t PlantRebuildAboard(const PlantRebuildCargoWorld &world, MapLayer layer = MapLayer::Invalid)
{
	uint32_t units = 0;
	// A loaded crossing is proved by real wagon cargo on the queried plane,
	// including a legitimate partly crossing consist, never its station sign.
	for (const Train *part = world.carrier; part != nullptr; part = part->Next()) {
		if (layer == MapLayer::Invalid || GetTileLayer(part->tile) == layer) units += part->cargo.StoredCount();
	}
	return units;
}

/** @param world Original actual carrier. @return Whether the entire stopped native chain is stored/hidden/empty, rather than only its front having entered a depot. */
inline bool PlantRebuildWholeEmptyHome(const PlantRebuildCargoWorld &world)
{
	if (!world.carrier->IsStoppedInDepot()) return false;
	// The original complete physical consist must return; an outside tail or
	// remaining packet cannot be hidden by the front's depot admission alone.
	for (const Train *part = world.carrier; part != nullptr; part = part->Next()) {
		if (!part->IsInDepot() || !part->vehstatus.Test(VehState::Hidden) || part->cargo.StoredCount() != 0) return false;
	}
	return true;
}

/** @param world Actual current fresh machinery. @return Whether the collector and both native generator construction owners have earned their full bills/work; physical service ports remain a separate later obligation. */
inline bool PlantRebuildAllConstructed(const PlantRebuildCargoWorld &world)
{
	return IsGeothermalCollectorOperational(world.machines[0]) && IsGeothermalGeneratorConstructionComplete(world.machines[1]) && IsGeothermalGeneratorConstructionComplete(world.machines[2]);
}

/** @param owner Actual native company. @return Current plus all retained native gross-income history, so a quarter turnover cannot hide a freight credit for construction supplies. */
inline Money PlantRebuildGrossIncome(CompanyID owner)
{
	const auto &company = *Company::Get(owner); Money income = company.cur_economy.income;
	// Include every real retained quarter. Purchase/transfer/final construction
	// purpose must not earn ordinary freight income on any accounting boundary.
	for (uint index = 0; index < company.num_valid_stat_ent; ++index) income += company.old_economy[index].income;
	return income;
}

/** @param world Original physical freight and fresh machinery. @param invoices Actual frozen native replacement purchase charge. @return Canonical observed carrier/packet/Industry/current-and-old-account/source data, extending material conservation without creating any owner. */
inline std::string CapturePlantRebuildDelivery(const PlantRebuildCargoWorld &world, Money invoices)
{
	std::string result = CapturePlantRebuildMaterials(world, invoices);
	/** @param name Unique observed native scalar name. @param value Actual native wire-style value; signed Money/position fields retain their original bit representation. Append observational metadata only. */
	const auto add = [&](std::string_view name, uint64_t value) { result += fmt::format("{}={},", name, value); };
	uint index = 0;
	// Preserve every original real carrier part, pose/order cursor and hold;
	// ordinary native movement alone may change these observations.
	for (const Train *part = world.carrier; part != nullptr; part = part->Next(), ++index) {
		const auto prefix = fmt::format("part{}_", index);
		add(prefix + "id", part->index.base()); add(prefix + "first", part->First()->index.base()); add(prefix + "engine", part->engine_type.base()); add(prefix + "owner", part->owner.base());
		add(prefix + "cargo", part->cargo_type); add(prefix + "units", part->cargo.StoredCount()); add(prefix + "capacity", part->cargo_cap);
		add(prefix + "tile", part->tile.base()); add(prefix + "layer", to_underlying(GetTileLayer(part->tile))); add(prefix + "x", part->x_pos); add(prefix + "y", part->y_pos); add(prefix + "z", part->z_pos);
		add(prefix + "direction", to_underlying(part->direction)); add(prefix + "track", part->track.base()); add(prefix + "speed", part->cur_speed); add(prefix + "subspeed", part->subspeed);
		add(prefix + "progress", part->progress); add(prefix + "status", part->vehstatus.base()); add(prefix + "flags", part->flags.base());
		add(prefix + "order", to_underlying(part->current_order.GetType())); add(prefix + "destination", part->current_order.GetDestination().base()); add(prefix + "order_index", part->cur_real_order_index);
		add(prefix + "dest_tile", part->dest_tile.base()); add(prefix + "profit", part->profit_this_year.base());
	}
	add("carrier_parts", index); add("aboard", PlantRebuildAboard(world)); add("whole_empty_home", PlantRebuildWholeEmptyHome(world));
	const auto &company = *Company::Get(world.carrier->owner); add("loan", company.current_loan.base()); add("quarters", company.num_valid_stat_ent); add("gross_income", PlantRebuildGrossIncome(world.carrier->owner).base());
	// All current/old native accounts and cargo buckets retain real quarter
	// rollovers and zero freight receipts, not an observer-computed payment.
	for (uint quarter = 0; quarter <= company.num_valid_stat_ent; ++quarter) {
		const auto &account = quarter == 0 ? company.cur_economy : company.old_economy[quarter - 1]; const auto prefix = quarter == 0 ? std::string{"current_"} : fmt::format("quarter{}_", quarter - 1);
		add(prefix + "income", account.income.base()); add(prefix + "expenses", account.expenses.base());
		for (const CargoType cargo : EnumRange(NUM_CARGO)) add(prefix + fmt::format("cargo{}", cargo), account.delivered_cargo[cargo]);
	}
	// Native Industry owns every actual received slot/history. Work/commitment
	// is a separate native receipt, never another physical material balance.
	for (uint machine = 0; machine < world.machines.size(); ++machine) {
		const auto &industry = *Industry::Get(world.machines[machine]); const auto prefix = fmt::format("machine{}_", machine);
		add(prefix + "id", industry.index.base()); add(prefix + "type", industry.type); add(prefix + "tile", industry.location.tile.base()); add(prefix + "counter", industry.counter); add(prefix + "history", industry.valid_history);
		for (uint slot = 0; slot < industry.accepted.size(); ++slot) {
			const auto &entry = industry.accepted[slot]; const auto name = prefix + fmt::format("input{}_", slot);
			add(name + "cargo", entry.cargo); add(name + "waiting", entry.waiting); add(name + "accumulated", entry.accumulated_waiting); add(name + "last", entry.last_accepted.base());
			if (entry.history != nullptr) {
				// Only the current month and native valid-history mask carry saved
				// receipt authority. Unwritten stale slots cannot become another bill
				// or a false ordinary-load mismatch during dependent file acceptance.
				for (uint month = 0; month < entry.history->size(); ++month) {
					if (month != THIS_MONTH && (industry.valid_history & (uint64_t{1} << month)) == 0) continue;
					add(name + fmt::format("h{}", month), (*entry.history)[month].accepted);
				}
			}
		}
		const auto *collector = GetGeothermalCollectorState(industry.index); const auto *generator = GetGeothermalGeneratorState(industry.index);
		add(prefix + "site", collector != nullptr ? collector->site.base() : generator->site.base());
		add(prefix + "last_work", collector != nullptr ? collector->last_work_tick : generator->last_work_tick);
		if (generator != nullptr) {
			add(prefix + "service_ticks", generator->active_service_ticks); add(prefix + "service_remainder", generator->service_remainder);
			add(prefix + "heat_consumer", generator->heat_consumer.base()); add(prefix + "electrical_producer", generator->electrical_producer.base());
		} else {
			add(prefix + "heat_producer", collector->heat_producer.base());
		}
	}
	// Actual paid packet IDs/purpose/age/first station survive native loading,
	// splitting and transfer. Ordinary unrelated cargo is still in full files.
	for (const CargoPacket *packet : CargoPacket::Iterate()) {
		if (packet->GetCompanySupplyOrder() == 0) continue;
		const auto prefix = fmt::format("packet{}_", packet->index.base()); add(prefix + "units", packet->Count()); add(prefix + "order", packet->GetCompanySupplyOrder());
		add(prefix + "shipped", packet->HasCompanySupplyShipped()); add(prefix + "source", packet->GetSource().id); add(prefix + "source_type", to_underlying(packet->GetSource().type));
		add(prefix + "first", packet->GetFirstStation().base()); add(prefix + "age", packet->GetPeriodsInTransit()); add(prefix + "feeder", packet->GetFeederShare().base());
	}
	const auto power = CaptureUtilityPowerState(); add("power_interval", power.interval); add("power_next_producer", power.next_producer); add("power_next_consumer", power.next_consumer);
	// Retired and naturally earned new meters retain their own native identity
	// and lifetime counters. A completed new collector may publish before wiring.
	for (const auto &producer : power.producers) {
		const auto prefix = fmt::format("producer{}_", producer.meter.producer_id.base()); add(prefix + "active", producer.active); add(prefix + "generated", producer.generated_lifetime_j);
		add(prefix + "consumed", producer.consumed_lifetime_j); add(prefix + "reserved", producer.reserved_j); add(prefix + "domain", to_underlying(producer.meter.domain));
	}
	return result;
}
#endif /* TESTS_PLANT_REBUILD_DELIVERY_FIXTURE_H */
