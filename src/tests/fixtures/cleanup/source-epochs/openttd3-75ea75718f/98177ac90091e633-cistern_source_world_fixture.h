/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file cistern_source_world_fixture.h Borrowed original native Cistern precursor participants and conserved observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * This view creates no participant, packet, industry stock, route or cargo definition.
 */
#ifndef TESTS_CISTERN_SOURCE_WORLD_FIXTURE_H
#define TESTS_CISTERN_SOURCE_WORLD_FIXTURE_H
#include <charconv>
#include "../train.h"
#include "../industry.h"
#include "../station_base.h"
#include "../company_base.h"
#include "../cargopacket.h"
#include "../layer_industry_processor.h"
#include "../multilayer_industry.h"
#include "../underground_vehicle.h"
#include "../excavation_state.h"
#include "movement_fixture.h"

/** Original ordinary-loaded participants; native pools remain the sole owners of every physical quantity. */
struct CisternSourceWorld {
	std::array<Train *, 4> parts{}; ///< Original Oil front/tanker and Salt front/wagon, in accepted report order.
	std::array<Industry *, 3> industries{}; ///< Actual original Oil Wells, CHEM processor and finite Salt mine.
	std::array<Station *, 3> stations{}; ///< Original Oil pickup, common Surface chemical berth and Underground Salt pickup.
	std::array<CargoType, 3> cargos{}; ///< Actual original OIL_, finite SALT and Liquid CHEM meanings.
	GeologyDepositID deposit{UINT32_MAX}; ///< Original geological reserve, separate from native industry identities.
};

/** @param name Required original report environment list. @param count Complete expected field count. @return Losslessly parsed unsigned original observations; missing/truncated fields fail before work. */
inline std::vector<uint64_t> CisternSourceValues(const char *name, size_t count)
{
	const char *text = std::getenv(name); REQUIRE(text != nullptr); const char *end = text + strlen(text);
	std::vector<uint64_t> values;
	// Every original field must parse completely. A partial list cannot select
	// a replacement pool owner or conceal an unverified original packet suffix.
	while (text < end) {
		uint64_t value = 0; const auto parsed = std::from_chars(text, end, value);
		REQUIRE(parsed.ec == std::errc{}); values.push_back(value);
		if (parsed.ptr == end) { text = end; break; }
		REQUIRE(*parsed.ptr == ','); text = parsed.ptr + 1; REQUIRE(text < end);
	}
	REQUIRE(values.size() == count); return values;
}

/** @param only_source Require the original complete source pool counts; an expanded consumer checks its own complete pool counts after reacquiring these same original participants. @return The exact original accepted native pool participants after ordinary application loading; no source fixture runs. */
inline CisternSourceWorld LoadCisternSourceWorld(bool only_source = true)
{
	CisternSourceWorld world;
	const auto parts = CisternSourceValues("OTTD_CISTERN_STOCK_PARTS", 4);
	const auto industries = CisternSourceValues("OTTD_CISTERN_STOCK_INDUSTRIES", 3);
	const auto stations = CisternSourceValues("OTTD_CISTERN_STOCK_STATIONS", 3);
	const auto deposit = CisternSourceValues("OTTD_CISTERN_STOCK_DEPOSIT", 1)[0]; REQUIRE(deposit < UINT32_MAX);
	world.deposit = GeologyDepositID{static_cast<uint32_t>(deposit)};
	// Source acceptance forbids extra participants. A later same-world route
	// retains these originals alongside explicitly checked new native owners;
	// its own full pool-count gate replaces only this source-only count check.
	if (only_source) {
		REQUIRE(Vehicle::GetNumItems() == parts.size()); REQUIRE(Industry::GetNumItems() == industries.size());
		REQUIRE(Station::GetNumItems() == stations.size());
	}
	// Reacquire every original actual identity and complete coupled consist;
	// a newly constructed train or equal scalar stock cannot replace the input.
	for (size_t index = 0; index < parts.size(); ++index) {
		REQUIRE(parts[index] < UINT32_MAX); world.parts[index] = Train::GetIfValid(VehicleID{static_cast<uint32_t>(parts[index])});
		REQUIRE(world.parts[index] != nullptr); REQUIRE_FALSE(world.parts[index]->vehstatus.Test(VehState::Crashed));
	}
	for (size_t index = 0; index < industries.size(); ++index) {
		REQUIRE(industries[index] < UINT16_MAX); REQUIRE(stations[index] < UINT16_MAX);
		world.industries[index] = Industry::GetIfValid(IndustryID{static_cast<uint16_t>(industries[index])}); REQUIRE(world.industries[index] != nullptr);
		world.stations[index] = Station::GetIfValid(StationID{static_cast<uint16_t>(stations[index])}); REQUIRE(world.stations[index] != nullptr);
	}
	world.cargos = {GetCargoTypeByLabel(CT_OIL), GetCargoTypeByLabel(CargoLabel{"SALT"}), GetCargoTypeByLabel(CargoLabel{"CHEM"})};
	for (const auto cargo : world.cargos) REQUIRE(cargo != INVALID_CARGO);
	for (uint index : {0, 2}) {
		REQUIRE(world.parts[index]->engine_type == GetUndergroundEngine(4)); REQUIRE(world.parts[index]->IsChainInDepot());
		REQUIRE(world.parts[index]->vehstatus.Test(VehState::Stopped)); REQUIRE(IsTrainOnItsTiles(world.parts[index]));
		REQUIRE(world.parts[index + 1]->First() == world.parts[index]); REQUIRE(world.parts[index + 1]->Next() == nullptr);
		REQUIRE(world.parts[index + 1]->cargo_type == world.cargos[index / 2]); REQUIRE(world.parts[index + 1]->cargo.StoredCount() == 0);
	}
	REQUIRE(world.parts[2 + 1]->engine_type == GetUndergroundEngine(5));
	const auto original_engine = CisternSourceValues("OTTD_CISTERN_STOCK_TANKER_ENGINE", 1)[0];
	REQUIRE(world.parts[1]->engine_type.base() == original_engine);
	REQUIRE(world.industries[0]->produced[0].cargo == world.cargos[0]);
	REQUIRE(world.industries[2]->type == GetLayerMineIndustryType(CargoLabel{"SALT"}));
	REQUIRE(world.industries[1]->type == GetLayerProcessorIndustryType(MultiLayerRecipeID{"CHEM"}));
	const auto *recipe = GetLayerProcessorRecipe(*world.industries[1]); REQUIRE(recipe != nullptr); REQUIRE(recipe->recipe.id == MultiLayerRecipeID{"CHEM"});
	const auto *record = GetGeologyWorldState().deposits.Get(world.deposit); REQUIRE(record != nullptr); REQUIRE(record->resource == CargoLabel{"SALT"});
	return world;
}

/** @param company Actual native company. @param cargo Original role being counted. @return Native cumulative freight income and delivered units across current plus valid historical quarters. */
inline auto CisternSourceAccounts(const Company &company, CargoType cargo)
{
	Money income = company.cur_economy.income; uint64_t delivered = company.cur_economy.delivered_cargo[cargo];
	// A real route may cross a native accounting quarter. Include retained
	// records instead of resetting time or treating bucket rotation as loss.
	for (uint index = 0; index < company.num_valid_stat_ent; ++index) {
		income += company.old_economy[index].income; delivered += company.old_economy[index].delivered_cargo[cargo];
	}
	return std::pair{income, delivered};
}

/** @param industry Actual native processor. @param cargo Exact raw input meaning. @return Its sole native unconsumed inventory, or zero for an absent slot. */
inline uint CisternSourceInput(const Industry &industry, CargoType cargo)
{
	const auto slot = std::ranges::find(industry.accepted, cargo, &Industry::AcceptedCargo::cargo);
	return slot == industry.accepted.end() ? 0 : slot->waiting;
}

/** @param world Original native source participants. @param role Zero for Oil or one for finite Salt. @return All remaining raw material plus genuine processor historical consumption, with each physical owner counted once. */
inline uint64_t CisternSourceRawTotal(const CisternSourceWorld &world, size_t role)
{
	const auto cargo = world.cargos[role]; const auto *processor = GetLayerProcessorState(world.industries[1]->index); REQUIRE(processor != nullptr);
	uint64_t total = CisternSourceInput(*world.industries[1], cargo) + processor->consumed.at(CargoSpec::Get(cargo)->label);
	// Native source buffers, all actual station queues and actual vehicle holds
	// form disjoint cargo owners. Reserved station units remain at that station
	// until StoredCount includes the physical vehicle load, avoiding double use.
	for (const auto *industry : Industry::Iterate()) for (const auto &slot : industry->produced) if (slot.cargo == cargo) total += slot.waiting;
	for (const auto *station : Station::Iterate()) total += station->goods[cargo].TotalCount();
	for (const auto *part : Vehicle::Iterate()) if (part->cargo_type == cargo) total += part->cargo.StoredCount();
	return total;
}
#endif /* TESTS_CISTERN_SOURCE_WORLD_FIXTURE_H */
