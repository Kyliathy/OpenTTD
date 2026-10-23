/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file forge_raw_delivery_fixture.h Borrowed original Forge input routes and native material/account observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups15FCC/15FDB/15FDC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Native inventories, recipe counters and quarters remain the sole owners; prepared shafts are not paid TBM proof.
 */
#ifndef TESTS_FORGE_RAW_DELIVERY_FIXTURE_H
#define TESTS_FORGE_RAW_DELIVERY_FIXTURE_H
#include "forge_raw_route_fixture.h"

/** @param raw_only Require the exact original4/3/3 pools/two portals; false permits later explicit modal consumers to validate their complete expanded pools separately. @return Exact ordinary-loaded original raw route participants, without constructing sources, facilities, orders or quantities. */
inline ForgeRawRouteWorld LoadForgeRawRouteWorld(bool raw_only = true)
{
	ForgeRawRouteWorld world; world.source = LoadForgeSourceWorld(false);
	if (raw_only) { REQUIRE(Vehicle::GetNumItems() == 4); REQUIRE(Industry::GetNumItems() == 3); REQUIRE(Station::GetNumItems() == 3); }
	const auto stations = ShieldSourceValues("OTTD_FORGE_RAW_STATIONS", 3), portals = ShieldSourceValues("OTTD_FORGE_RAW_PORTALS", 2);
	// Actual original station identities, ownership and layer geometry must
	// survive; matching cargo totals cannot substitute reconstructed facilities.
	for (size_t index = 0; index < world.stations.size(); ++index) {
		REQUIRE(stations[index] < UINT16_MAX); world.stations[index] = Station::GetIfValid(StationID{static_cast<uint16_t>(stations[index])});
		REQUIRE(world.stations[index] != nullptr); REQUIRE(world.stations[index]->owner == _current_company);
		REQUIRE(GetTileLayer(world.stations[index]->xy) == (index == 2 ? MapLayer::Surface : MapLayer::Underground));
	}
	REQUIRE(world.stations[0] != world.stations[1]); REQUIRE(world.stations[0] != world.stations[2]); REQUIRE(world.stations[1] != world.stations[2]);
	const auto records = CaptureLayerPortalWorkRecords();
	if (raw_only) REQUIRE(records.size() == portals.size());
	// Later paid modal preparation may add separate shafts, but each original
	// raw pair must still exist with its original native Connected identity.
	for (const auto key : portals) {
		/** @param row Actual restored native pair. @return Whether it retains this original accepted raw-route key. */
		const auto matches = [key](const LayerPortalWorkRecord &row) { return row.pair_key.base() == key; };
		const auto record = std::ranges::find_if(records, matches);
		REQUIRE(record != records.end()); REQUIRE(record->state == LayerPortalWorkState::Connected);
	}
	for (const auto tile : world.far_depots) { REQUIRE(IsRailDepotTile(tile)); REQUIRE(::Depot::GetByTile(tile) != nullptr); }
	return world;
}

/** @param industry Actual native receiving inventory. @param cargo Original accepted meaning. @return Existing input stock, without creating a missing slot. */
inline uint ForgeRawInput(const Industry &industry, CargoType cargo)
{
	const auto slot = std::ranges::find(industry.accepted, cargo, &Industry::AcceptedCargo::cargo);
	return slot == industry.accepted.end() ? 0 : slot->waiting;
}

/** @param company Actual accounting owner. @param cargo Original raw meaning. @return Native income and delivered cargo across current and every valid historical quarter. */
inline auto ForgeRawAccounts(const Company &company, CargoType cargo)
{
	Money income = company.cur_economy.income; uint64_t delivered = company.cur_economy.delivered_cargo[cargo];
	// Calendar rollover retains native historical owners. A private cumulative
	// balance or a frozen date cannot replace these real delivery/payment facts.
	for (uint index = 0; index < company.num_valid_stat_ent; ++index) {
		income += company.old_economy[index].income; delivered += company.old_economy[index].delivered_cargo[cargo];
	}
	return std::pair{income, delivered};
}

/** @param cargo Actual cargo meaning. @return Every physical industry input/output, station queue and native stored hold, counted once. */
inline uint64_t ForgeRawPhysical(CargoType cargo)
{
	uint64_t physical = 0;
	// These disjoint native buffers retain the actual goods; historical
	// dispatch, intake, recipe consumption and company payment are not stores.
	for (const auto *industry : Industry::Iterate()) {
		for (const auto &entry : industry->produced) if (entry.cargo == cargo) physical += entry.waiting;
		for (const auto &entry : industry->accepted) if (entry.cargo == cargo) physical += entry.waiting;
	}
	for (const auto *station : Station::Iterate()) physical += station->goods[cargo].TotalCount();
	for (const auto *part : Vehicle::Iterate()) if (part->cargo_type == cargo) physical += part->cargo.StoredCount();
	return physical;
}

/** @param world Borrowed original source/routes. @param reserves Original generated CORE/Coal reserves. @param produced_only Require undispatched RFMT/raw packets only; false admits actual CFMT RFMT packets into the later unconsumed ELEC input route, with every physical unit still conserved. Conserve ongoing extraction, exact native batches and raw receipt history independently per meaning. */
inline void CheckForgeRawDelivery(const ForgeRawRouteWorld &world, const std::array<uint64_t, 2> &reserves, bool produced_only = true)
{
	const auto &source = world.source; const auto *processor = GetLayerProcessorState(source.industries[2]->index); REQUIRE(processor != nullptr);
	REQUIRE(processor->recipe == MultiLayerRecipeID{"CFMT"}); const auto batches = processor->batches.at(MultiLayerRecipeID{"CFMT"});
	REQUIRE(processor->consumed.at(CargoLabel{"CORE"}) == batches * 4); REQUIRE(processor->consumed.at(CargoLabel{"COAL"}) == batches);
	REQUIRE(processor->produced.at(CargoLabel{"RFMT"}) == batches * 2);
	if (produced_only) REQUIRE(processor->dispatched == 0);
	uint64_t output = 0;
	for (const auto &entry : source.industries[2]->produced) if (entry.cargo == source.cargos[2]) output += entry.waiting;
	REQUIRE(output + processor->dispatched == batches * 2);
	REQUIRE(ForgeRawPhysical(source.cargos[2]) == batches * 2);
	// Both raw labels keep their own reserve, extraction, physical inventory,
	// recipe debit and paid receipt; excess Coal remains genuine unused stock.
	for (size_t index = 0; index < reserves.size(); ++index) {
		const auto cargo = source.cargos[index]; const auto label = CargoSpec::Get(cargo)->label;
		const auto *deposit = GetGeologyWorldState().deposits.Get(source.deposits[index]); REQUIRE(deposit != nullptr);
		const auto *mine = GetLayerMineState(source.industries[index]->index); REQUIRE(mine != nullptr);
		REQUIRE(deposit->initial_reserve == reserves[index]); REQUIRE(deposit->remaining_reserve + ForgeRawPhysical(cargo) + processor->consumed.at(label) == reserves[index]);
		REQUIRE(mine->extracted == reserves[index] - deposit->remaining_reserve); REQUIRE(mine->extracted == mine->dispatched + source.industries[index]->produced[0].waiting);
		REQUIRE(ForgeRawAccounts(*Company::Get(source.parts[0]->owner), cargo).second == ForgeRawInput(*source.industries[2], cargo) + processor->consumed.at(label));
	}
	// Every real packet belongs to an actual original raw mine/pickup. Only
	// an explicit later modal consumer may also carry genuine CFMT RFMT from
	// its actual Surface pickup; splitting/aging retains paying provenance.
	for (const auto *packet : CargoPacket::Iterate()) {
		if (!produced_only && packet->GetSource() == Source{source.industries[2]->index, SourceType::Industry}) {
			REQUIRE(packet->GetFirstStation() == world.stations[2]->index);
		} else {
			const bool copper = packet->GetSource() == Source{source.industries[0]->index, SourceType::Industry};
			if (!copper) REQUIRE(packet->GetSource() == Source{source.industries[1]->index, SourceType::Industry});
			REQUIRE(packet->GetFirstStation() == world.stations[copper ? 0 : 1]->index);
		}
		REQUIRE(packet->GetCompanySupplyOrder() == 0);
	}
}

/** @param world Exact original paid participants. Advance every native clock/front and industry once, then dispatch only genuine raw output through earned station service. */
inline void RunForgeRawOpportunity(const ForgeRawRouteWorld &world)
{
	// Ordinary startup loads a paused review world. Its caller must explicitly
	// resume native gameplay before advancing clocks, otherwise mines correctly
	// refuse production/dispatch while a manually driven vehicle can still move.
	REQUIRE_FALSE(_pause_mode.Any()); REQUIRE(_game_mode == GameMode::Normal);
	RunShieldSourceOpportunity();
	for (size_t index = 0; index < world.source.deposits.size(); ++index) {
		DispatchLayerMineCargo(*world.source.industries[index]);
		REQUIRE(IsTrainOnItsTiles(world.source.parts[index * 2])); REQUIRE_FALSE(world.source.parts[index * 2]->vehstatus.Test(VehState::Crashed));
	}
}

/** @param world Original paid source/route owners. @return Read-only complete source observations plus station eligibility and every genuine packet for pure saving and later ordinary file consumers. */
inline std::string CaptureForgeRawDelivery(const ForgeRawRouteWorld &world)
{
	std::string result = CaptureForgeSourceWorld(world.source);
	/** @param name Unique native field name. @param value Exact integer bits, including losslessly transported signed Money. */
	const auto add = [&](std::string_view name, uint64_t value) { result += fmt::format("{}={},", name, value); };
	// Real station queues/service and packet identity/aging accompany source,
	// recipe, history, pose, order, cash and clock observations from the source.
	for (size_t index = 0; index < world.stations.size(); ++index) {
		const auto &station = *world.stations[index]; const auto prefix = fmt::format("raw_station{}_", index);
		add(prefix + "id", station.index.base()); add(prefix + "tile", station.xy.base()); add(prefix + "owner", station.owner.base());
		for (const auto cargo : world.source.cargos) {
			const auto &entry = station.goods[cargo]; const auto name = prefix + fmt::format("cargo{}_", cargo);
			add(name + "units", entry.TotalCount()); add(name + "rating", entry.rating); add(name + "speed", entry.last_speed); add(name + "fraction", entry.amount_fract); add(name + "status", entry.status.base());
		}
	}
	for (const auto *packet : CargoPacket::Iterate()) {
		const auto prefix = fmt::format("packet{}_", packet->index.base()); add(prefix + "units", packet->Count()); add(prefix + "first", packet->GetFirstStation().base());
		add(prefix + "source_tile", packet->GetSourceTile().base()); add(prefix + "copper", packet->GetSource() == Source{world.source.industries[0]->index, SourceType::Industry});
		add(prefix + "purpose", packet->GetCompanySupplyOrder()); add(prefix + "age", packet->GetPeriodsInTransit()); add(prefix + "feeder", packet->GetFeederShare().base());
	}
	return result;
}
#endif /* TESTS_FORGE_RAW_DELIVERY_FIXTURE_H */
