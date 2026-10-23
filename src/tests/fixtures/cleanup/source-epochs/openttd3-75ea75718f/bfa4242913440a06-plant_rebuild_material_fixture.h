/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file plant_rebuild_material_fixture.h Borrowed original paid reconstruction participants and native material observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups1CCCB/1CCCC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Retired bills are historical consumption from sealed1CB, never live inventory. Native queues/holds/Industry slots own every remaining unit.
 */
#ifndef TESTS_PLANT_REBUILD_MATERIAL_FIXTURE_H
#define TESTS_PLANT_REBUILD_MATERIAL_FIXTURE_H
#include "geothermal_first_plant_fixture.h"
#include "../cargo_type.h"
#include <set>

static constexpr std::array PLANT_REBUILD_LABELS{CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}}; ///< Frozen existing construction meanings; runtime slots/prices remain native queries.
static constexpr std::array PLANT_REBUILD_KINDS{MultiLayerGeothermalConstructionKind::Collector, MultiLayerGeothermalConstructionKind::Powerhouse,
	MultiLayerGeothermalConstructionKind::MagmaPowerhouse}; ///< Original retired and actual fresh role order, not a new content registry.

/** Borrowed actual native reconstruction roles; no participant, inventory, geometry, order or resource quantity is created here. */
struct PlantRebuildCargoWorld {
	std::array<IndustryID, 3> machines; ///< Native fresh collector/VentTap/Magma incarnations bound to claims3/4/5.
	Train *carrier = nullptr; ///< Original actual three-stop/four-hold construction train, not a replacement freight fixture.
	StationID origin = StationID::Invalid(); ///< First original manual railway stop, the actual paid Surface supplier.
};

/** @param kind Existing canonical construction family. @param label Existing native physical input meaning. @return Frozen whole-bill quantity for this meaning, or zero when absent; this query creates no material. */
inline uint32_t PlantRebuildBill(MultiLayerGeothermalConstructionKind kind, CargoLabel label)
{
	const auto *terms = GetMultiLayerGeothermalConstruction(kind); REQUIRE(terms != nullptr);
	for (const auto &input : terms->inputs) if (input.label == label) return input.quantity;
	return 0;
}

/** @param label Existing native physical input meaning. @return Required new bill for all three actual replacement roles; the same amount was historically consumed by their verified demolished originals. */
inline uint32_t PlantRebuildWholeBill(CargoLabel label)
{
	uint32_t quantity = 0;
	// These are three distinct original/fresh bills. Their historical retired
	// quantities never become available stock for the new funded machinery.
	for (const auto kind : PLANT_REBUILD_KINDS) quantity += PlantRebuildBill(kind, label);
	return quantity;
}

/** @return Actual original fresh machinery, complete retained carrier and its Surface supplier borrowed after ordinary loading; no fixture builder or ownership restoration is permitted. */
inline PlantRebuildCargoWorld LoadPlantRebuildCargoWorld()
{
	PlantRebuildCargoWorld world; const auto claims = CaptureGeothermalSiteClaims(); REQUIRE(claims.size() == 6);
	REQUIRE(GetGeologyWorldState().vents.size() == 1); const auto vent = GetGeologyWorldState().vents.front(); REQUIRE(vent.thermal_capacity_w == 1000000);
	// Sealed1CB proves the first three consumed bills. Actual1CCB fresh claims
	// reuse only their geometry, while native construction keeps separate work.
	for (uint index = 0; index < 3; ++index) {
		const auto &old = claims[index], &fresh = claims[index + 3]; REQUIRE_FALSE(old.active); REQUIRE_FALSE(fresh.active);
		REQUIRE(old.site.kind == PLANT_REBUILD_KINDS[index]); REQUIRE(fresh.site.kind == old.site.kind);
		REQUIRE(old.site.vent == vent.id); REQUIRE(fresh.site.vent == vent.id); REQUIRE(fresh.id > old.id);
		REQUIRE(fresh.site.tile == old.site.tile); REQUIRE(fresh.site.endpoint == old.site.endpoint); REQUIRE(fresh.site.owner == _current_company);
		const auto *industry = Industry::GetByTile(fresh.site.tile); REQUIRE(industry != nullptr); world.machines[index] = industry->index;
		if (index == 0) {
			const auto *state = GetGeothermalCollectorState(industry->index); REQUIRE(state != nullptr); REQUIRE(state->site == fresh.id);
		} else {
			const auto *state = GetGeothermalGeneratorState(industry->index); REQUIRE(state != nullptr); REQUIRE(state->site == fresh.id);
			REQUIRE(GetMultiLayerGeothermalGeneratorTerms(state->model)->construction == fresh.site.kind);
		}
	}
	// The paid spoil hauler uses the same native front model but has one stop.
	// The retained three-stop construction train alone owns these four inputs;
	// choosing another consist would bypass the original paid transport leg.
	for (Train *train : Train::Iterate()) {
		if (!train->IsFrontEngine() || train->engine_type != GetUndergroundEngine(4) || train->GetNumManualOrders() != 3 || train->owner != _current_company) continue;
		REQUIRE(world.carrier == nullptr); world.carrier = train;
	}
	REQUIRE(world.carrier != nullptr);
	for (const Order &order : world.carrier->Orders()) {
		if (order.IsType(OT_GOTO_STATION)) { world.origin = order.GetDestination().ToStationID(); break; }
	}
	const auto *station = Station::GetIfValid(world.origin); REQUIRE(station != nullptr); REQUIRE(station->owner == _current_company);
	REQUIRE(GetTileLayer(station->xy) == MapLayer::Surface); REQUIRE(station->facilities.Test(StationFacility::Train));
	uint parts = 0; std::set<CargoType> holds;
	// Four separately refitted native wagons, each with real64-unit capacity,
	// carry the complete bill; the locomotive cannot supply another inventory.
	for (const Train *part = world.carrier; part != nullptr; part = part->Next()) {
		++parts;
		if (part == world.carrier) continue;
		REQUIRE(part->engine_type == GetUndergroundEngine(5)); REQUIRE(part->cargo_cap == 64); holds.insert(part->cargo_type);
	}
	REQUIRE(parts == 5); REQUIRE(holds.size() == 4);
	for (const auto label : PLANT_REBUILD_LABELS) REQUIRE(holds.contains(GetCargoTypeByLabel(label)));
	return world;
}

/** Actual material accounting observations only; native CargoPackets and Industry stock remain the sole physical quantities. */
struct PlantRebuildMaterialLedger {
	uint32_t bought = 0; ///< Actual monotonic paid Surface history, including consumed retired material.
	uint32_t dispatched = 0; ///< Actual historical departure, never another physical unit.
	uint32_t physical = 0; ///< Sum of actual native station/vehicle/Industry custody.
	uint32_t retired = 0; ///< Historical original whole bills proved consumed before native demolition.
	uint32_t live_consumed = 0; ///< Actual current native committed whole bills, including retained original risers.
	uint32_t source = 0; ///< Actual physical stock still at the original Surface supplier.
};

/** @param world Original actual reconstruction roles and supplier. @param label Native physical material meaning. @return Reconciled paid/physical/retired/live consumed observations; no inventory or bill is assigned to gameplay. */
inline PlantRebuildMaterialLedger CheckPlantRebuildMaterial(const PlantRebuildCargoWorld &world, CargoLabel label)
{
	PlantRebuildMaterialLedger ledger; const auto cargo = GetCargoTypeByLabel(label); REQUIRE(cargo != INVALID_CARGO);
	// Monotonic paid history survives original machinery destruction. Its
	// consumed92-unit attribution cannot be bought again for free or counted
	// as physical stock of the new native incarnations.
	for (const auto &order : GetSurfaceSupplyOrderHistory()) {
		if (order.label != label) continue;
		REQUIRE(order.station == world.origin); REQUIRE(order.refunded == 0); ledger.bought += order.quantity; ledger.dispatched += order.dispatched;
	}
	for (const Station *station : Station::Iterate()) ledger.physical += station->goods[cargo].TotalCount();
	for (const Vehicle *vehicle : Vehicle::Iterate()) if (vehicle->cargo_type == cargo) ledger.physical += vehicle->cargo.TotalCount();
	for (const Industry *industry : Industry::Iterate()) {
		const auto slot = industry->GetCargoAccepted(cargo); if (slot != industry->accepted.end()) ledger.physical += slot->waiting;
	}
	ledger.retired = PlantRebuildWholeBill(label);
	// Only actual live native material-commitment receipts count as fresh
	// consumption. Missing/unfinished machinery owns no implied consumed bill.
	for (const auto &state : CaptureGeothermalCollectors()) if (state.materials_committed) ledger.live_consumed += PlantRebuildBill(MultiLayerGeothermalConstructionKind::Collector, label);
	for (const auto &state : CaptureGeothermalGenerators()) if (state.materials_committed) ledger.live_consumed += PlantRebuildBill(GetMultiLayerGeothermalGeneratorTerms(state.model)->construction, label);
	for (const auto &state : CaptureGeothermalRisers()) if (state.materials_committed) ledger.live_consumed += PlantRebuildBill(MultiLayerGeothermalConstructionKind::Riser, label);
	ledger.source = Station::Get(world.origin)->goods[cargo].TotalCount();
	CHECK(ledger.bought == ledger.physical + ledger.retired + ledger.live_consumed); CHECK(ledger.dispatched <= ledger.bought);
	CHECK(ValidateSurfaceSupplyState(CaptureSurfaceSupplyState(), true) == SurfaceSupplyError::None);
	return ledger;
}

/** @param world Original actual reconstruction participants. @param invoices Actual new native purchase debits only. @return Flat original/fresh construction, retained source/spoil/account and paid material observations; no simulation opportunity is created. */
inline std::string CapturePlantRebuildMaterials(const PlantRebuildCargoWorld &world, Money invoices)
{
	std::string result = fmt::format("tick={},year={},cash={},invoices={},geology={},projects={},exchange={},risers={},utility={},supply={},orders={},next_order={},origin={},carrier={},vehicles={},industries={},packets={},",
		TimerGameTick::counter, TimerGameCalendar::year.base(), Company::Get(_current_company)->money.base(), invoices.base(), ComputeGeologyStateDigest(GetGeologyWorldState()),
		ComputeTBMProjectDigest(), ComputeTBMExchangeDigest(), ComputeGeothermalRiserDigest(), ComputeUtilityPowerDigest(), ComputeSurfaceSupplyDigest(), GetSurfaceSupplyOrderHistory().size(),
		GetNextSurfaceSupplyOrderID().base(), world.origin.base(), world.carrier->index.base(), Vehicle::GetNumItems(), Industry::GetNumItems(), CargoPacket::GetNumItems());
	// Every actual fresh machine retains its native work/material receipt;
	// historical consumed bills are reported separately for each material.
	for (uint index = 0; index < world.machines.size(); ++index) {
		const auto *collector = GetGeothermalCollectorState(world.machines[index]); const auto *generator = GetGeothermalGeneratorState(world.machines[index]);
		REQUIRE((collector != nullptr || generator != nullptr));
		result += fmt::format("industry{}={},work{}={},committed{}={},", index, world.machines[index].base(), index,
			collector != nullptr ? collector->work_ticks : generator->work_ticks, index, static_cast<uint>(collector != nullptr ? collector->materials_committed : generator->materials_committed));
	}
	for (uint index = 0; index < PLANT_REBUILD_LABELS.size(); ++index) {
		const auto ledger = CheckPlantRebuildMaterial(world, PLANT_REBUILD_LABELS[index]);
		result += fmt::format("cargo{}={},bought{}={},dispatched{}={},physical{}={},retired{}={},live_consumed{}={},source{}={},",
			index, GetCargoTypeByLabel(PLANT_REBUILD_LABELS[index]), index, ledger.bought, index, ledger.dispatched, index, ledger.physical, index, ledger.retired, index, ledger.live_consumed, index, ledger.source);
	}
	return result;
}
#endif /* TESTS_PLANT_REBUILD_MATERIAL_FIXTURE_H */
